#include "Scheduler.h"

#include "Utils/LoggingSystem.h"

namespace esx {

	U64 Scheduler::sProgressClock = 0;
	SchedulerEventContainer Scheduler::sEvents = {};
	SchedulerEventHandlerContainer Scheduler::sEventHandlers;
	BIT Scheduler::sStopCurrentEvent = ESX_FALSE;
	U64 Scheduler::sEventSerial = 1;

	void Scheduler::ScheduleEvent(SchedulerEvent& schedulerEvent)
	{
		schedulerEvent.Id = sEventSerial++;

		// Reverse search: rescheduled periodic events typically belong near the end
		auto rit = std::find_if(sEvents.rbegin(), sEvents.rend(), [&](const SchedulerEvent& ev) {
			if (schedulerEvent.ClockTarget > ev.ClockTarget) return true;
			if (schedulerEvent.ClockTarget < ev.ClockTarget) return false;
			// Same clock: lower priority value = higher priority (runs first)
			if (schedulerEvent.Priority != ev.Priority) return schedulerEvent.Priority > ev.Priority;
			// Same priority: higher enum value runs first
			return static_cast<U8>(schedulerEvent.Type) < static_cast<U8>(ev.Type);
		});

		sEvents.insert(rit.base(), schedulerEvent);
	}

	void Scheduler::UnScheduleAllEvents(SchedulerEventType type)
	{
		std::erase_if(sEvents, [&](const SchedulerEvent& ev) { return ev.Type == type; });
	}

	void Scheduler::UnScheduleEventTypes(std::initializer_list<SchedulerEventType> types)
	{
		std::erase_if(sEvents, [&](const SchedulerEvent& ev) {
			for (auto t : types) {
				if (ev.Type == t) return true;
			}
			return false;
		});
	}

	Optional<SchedulerEvent*> Scheduler::NextEventOfType(SchedulerEventType type, U64 idToExclude)
	{
		Optional<SchedulerEvent*> result = {};
		auto it = std::find_if(sEvents.begin(), sEvents.end(), [&](const SchedulerEvent& ev) { return ev.Type == type && ev.Id != idToExclude; });
		if (it != sEvents.end()) {
			result.emplace(&*it);
		}
		return result;
	}

	const SchedulerEvent& Scheduler::NextEvent()
	{
		return sEvents.front();
	}

	BIT Scheduler::CurrentEventHasBeenStopped()
	{
		return sStopCurrentEvent;
	}

	void Scheduler::StopCurrentEvent()
	{
		sStopCurrentEvent = ESX_TRUE;
	}

	void Scheduler::ExecuteEvent()
	{
		SchedulerEvent& currentFront = sEvents.front();
		sStopCurrentEvent = ESX_FALSE;

		const auto& handlers = sEventHandlers[static_cast<size_t>(currentFront.Type)];
		for (const SchedulerEventHandler& evHandler : handlers) {
			evHandler(currentFront);
			if (sStopCurrentEvent) break;
		}
	}

	void Scheduler::Progress()
	{
		SchedulerEvent currentFront = std::move(sEvents.front());
		sEvents.pop_front();

		if (currentFront.Reschedule) {
			SchedulerEvent rescheduleEvent;
			rescheduleEvent.Type = currentFront.Type;
			rescheduleEvent.ClockStart = currentFront.ClockTarget;
			rescheduleEvent.ClockTarget = currentFront.ClockTarget + currentFront.RescheduleClocks;
			rescheduleEvent.Reschedule = ESX_TRUE;
			rescheduleEvent.RescheduleClocks = currentFront.RescheduleClocks;
			rescheduleEvent.Priority = currentFront.Priority;
			ScheduleEvent(rescheduleEvent);
		}

		sProgressClock = currentFront.ClockTarget;
	}

	void Scheduler::AddSchedulerEventHandler(SchedulerEventType type, const SchedulerEventHandler& handler)
	{
		sEventHandlers[static_cast<size_t>(type)].push_back(handler);
	}

	BIT Scheduler::HasEvents()
	{
		return sEvents.empty() ? ESX_FALSE : ESX_TRUE;
	}

}
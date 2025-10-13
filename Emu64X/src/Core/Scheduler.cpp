#include "Scheduler.h"

#include "Utils/LoggingSystem.h"

namespace esx {

	U64 Scheduler::sProgressClock = 0;
	SchedulerEventContainer Scheduler::sEvents = {};
	SchedulerEventHandlerContainer Scheduler::sEventHandlers = {};
	BIT Scheduler::sStopCurrentEvent = ESX_FALSE;
	U64 Scheduler::sEventSerial = 1;

	void Scheduler::ScheduleEvent(SchedulerEvent& schedulerEvent)
	{
		schedulerEvent.Id = sEventSerial++;
		auto it = std::find_if(sEvents.begin(), sEvents.end(), [&](const SchedulerEvent& ev) { return schedulerEvent.ClockTarget < ev.ClockTarget || (schedulerEvent.ClockTarget == ev.ClockTarget && static_cast<U8>(schedulerEvent.Type) > static_cast<U8>(ev.Type) && schedulerEvent.Priority < ev.Priority); });
		if (it != sEvents.end()) {
			sEvents.insert(it, schedulerEvent);
		} else {
			sEvents.push_back(schedulerEvent);
		}
	}

	void Scheduler::UnScheduleAllEvents(SchedulerEventType type)
	{
		std::erase_if(sEvents, [&](const SchedulerEvent& ev) { return ev.Type == type; });
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

	void Scheduler::ExecuteEvent()
	{
		SchedulerEvent& currentFront = sEvents.front();

		for (const SchedulerEventHandler& evHandler : sEventHandlers[currentFront.Type]) {
			evHandler(currentFront);
		}
	}

	void Scheduler::Progress()
	{

		const SchedulerEvent& currentFront = sEvents.front();

		if (currentFront.Type != currentFront.Type) {
			ESX_CORE_LOG_ERROR("Errore Fatale");
		}

		if (currentFront.Reschedule) {
			SchedulerEvent rescheduleEvent = currentFront;
			rescheduleEvent.ClockStart = rescheduleEvent.ClockTarget;
			rescheduleEvent.ClockTarget = rescheduleEvent.ClockStart + rescheduleEvent.RescheduleClocks;
			ScheduleEvent(rescheduleEvent);
		}

		sProgressClock = currentFront.ClockTarget;

		sEvents.pop_front();
	}

	void Scheduler::AddSchedulerEventHandler(SchedulerEventType type, const SchedulerEventHandler& handler)
	{
		sEventHandlers[type].push_back(handler);
	}

	BIT Scheduler::HasEvents()
	{
		return sEvents.empty() ? ESX_FALSE : ESX_TRUE;
	}

}
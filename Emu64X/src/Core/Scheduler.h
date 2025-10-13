#pragma once

#include "Base/Base.h"

namespace esx {

	enum class SchedulerEventType {
		None,
		GPUFrameStart,
		GPUScanlineStart,
		GPUStartVBlank,
		GPUEndVBlank,
		GPUStartHBlank,
		GPUEndHBlank,
		PIDMADone,
		SIDMADone,
		AIDMADone,
		AIDMAStart,
		SPDMADone,
		DPDMADone
	};

	struct SchedulerEvent {
		U64 Id = 0;
		SchedulerEventType Type = SchedulerEventType::None;
		U64 ClockStart = 0;
		U64 ClockTarget = 0;
		BIT Reschedule = ESX_FALSE;
		U64 RescheduleClocks = 0;
		Vector<U8> UserData = {};
		U8 ReadPointer = 0;
		I32 Priority = 0;

		template<typename T>
		void Write(const T& Data) {
			size_t writeP = UserData.size();
			UserData.resize(UserData.size() + sizeof(T));
			std::memcpy(UserData.data() + writeP, reinterpret_cast<const U8*>(&Data), sizeof(T));
		}

		template<typename T>
		T Read() {
			T value = *reinterpret_cast<const T*>(UserData.data() + ReadPointer);
			ReadPointer += sizeof(T);
			return value;
		}

		void Clear() {
			UserData.resize(0);
		}
	};

	struct SchedulerCompare {
		bool operator()(const SchedulerEvent& l, const SchedulerEvent& r) const { return l.ClockTarget > r.ClockTarget || (l.ClockTarget == r.ClockTarget && static_cast<U8>(l.Type) > static_cast<U8>(r.Type)); }
	};

	using SchedulerEventHandler = Function<void(SchedulerEvent&)>;
	using SchedulerEventContainer = Deque<SchedulerEvent>;
	using SchedulerEventHandlerContainer = UnorderedMap<SchedulerEventType, Vector<SchedulerEventHandler>>;

	class Scheduler {
	public:
		Scheduler() = delete;
		~Scheduler() = default;

		static void ScheduleEvent(SchedulerEvent& schedulerEvent);
		static void UnScheduleAllEvents(SchedulerEventType type);
		static Optional<SchedulerEvent*> NextEventOfType(SchedulerEventType type, U64 idToExclude = 0);
		static const SchedulerEvent& NextEvent();
		static BIT CurrentEventHasBeenStopped();
		static void ExecuteEvent();
		static void Progress();
		static void AddSchedulerEventHandler(SchedulerEventType type, const SchedulerEventHandler& handler);
		static BIT HasEvents();
	private:
		static U64 sProgressClock;
		static SchedulerEventContainer sEvents;
		static SchedulerEventHandlerContainer sEventHandlers;
		static BIT sStopCurrentEvent;
		static U64 sEventSerial;

	};

}
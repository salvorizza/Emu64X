#pragma once

#include "Base/Base.h"

namespace esx {

	enum class SchedulerEventType : U8 {
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
		DPDMADone,
		EventTypeCount
	};

	static constexpr size_t SchedulerInlineBufferSize = 32;

	struct SchedulerEvent {
		U64 Id = 0;
		SchedulerEventType Type = SchedulerEventType::None;
		U64 ClockStart = 0;
		U64 ClockTarget = 0;
		BIT Reschedule = ESX_FALSE;
		U64 RescheduleClocks = 0;
		I32 Priority = 0;

		Array<U8, SchedulerInlineBufferSize> InlineData = {};
		Vector<U8> HeapData = {};
		U16 DataSize = 0;
		U16 ReadPointer = 0;

		template<typename T>
		void Write(const T& Data) {
			const U16 writeP = DataSize;
			DataSize += static_cast<U16>(sizeof(T));
			if (DataSize <= SchedulerInlineBufferSize) {
				std::memcpy(InlineData.data() + writeP, reinterpret_cast<const U8*>(&Data), sizeof(T));
			} else {
				if (HeapData.size() < DataSize) HeapData.resize(DataSize);
				if (writeP < SchedulerInlineBufferSize) {
					std::memcpy(HeapData.data(), InlineData.data(), writeP);
				}
				std::memcpy(HeapData.data() + writeP, reinterpret_cast<const U8*>(&Data), sizeof(T));
			}
		}

		template<typename T>
		T Read() {
			const U8* src = (DataSize <= SchedulerInlineBufferSize) ? InlineData.data() : HeapData.data();
			T value = *reinterpret_cast<const T*>(src + ReadPointer);
			ReadPointer += static_cast<U16>(sizeof(T));
			return value;
		}

		void Clear() {
			DataSize = 0;
			ReadPointer = 0;
		}
	};

	using SchedulerEventHandler = Function<void(SchedulerEvent&)>;
	using SchedulerEventContainer = Deque<SchedulerEvent>;
	using SchedulerEventHandlerContainer = Array<Vector<SchedulerEventHandler>, static_cast<size_t>(SchedulerEventType::EventTypeCount)>;

	class Scheduler {
	public:
		Scheduler() = delete;
		~Scheduler() = default;

		static void ScheduleEvent(SchedulerEvent& schedulerEvent);
		static void UnScheduleAllEvents(SchedulerEventType type);
		static void UnScheduleEventTypes(std::initializer_list<SchedulerEventType> types);
		static Optional<SchedulerEvent*> NextEventOfType(SchedulerEventType type, U64 idToExclude = 0);
		static const SchedulerEvent& NextEvent();
		static BIT CurrentEventHasBeenStopped();
		static void StopCurrentEvent();
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
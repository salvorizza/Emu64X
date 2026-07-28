#include "VideoInterface.h"

#include "../RCP.h"
#include "Core/MIPS/VR4300/VR4300.h"
#include "Core/Scheduler.h"

namespace esx {

	VideoInterface::VideoInterface(RCP* rcp)
		: mRCP(rcp)
	{
		Scheduler::AddSchedulerEventHandler(SchedulerEventType::GPUFrameStart, [&](const SchedulerEvent& ev) {
			if (VI_V_TOTAL.get<layouts::VI_V_TOTAL_Register::V_TOTAL>() & 0x1) {
				VI_V_CURRENT.set<layouts::VI_V_CURRENT_Register::V_FIELD>(0);
			} else {
				VI_V_CURRENT.set<layouts::VI_V_CURRENT_Register::V_FIELD>(!VI_V_CURRENT.get<layouts::VI_V_CURRENT_Register::V_FIELD>());
			}

			VI_V_CURRENT.set<layouts::VI_V_CURRENT_Register::V_CURRENT>(0);

			mFirstLine = ESX_TRUE;
			mCountLine = ESX_FALSE;
		});

		Scheduler::AddSchedulerEventHandler(SchedulerEventType::GPUScanlineStart, [&](const SchedulerEvent& ev) {
			if (mFirstLine == ESX_FALSE) {
				U16 currentLine = VI_V_CURRENT.get<layouts::VI_V_CURRENT_Register::V_CURRENT>();
				if (VI_V_TOTAL.get<layouts::VI_V_TOTAL_Register::V_TOTAL>() & 0x1) {
					currentLine += (mCountLine ? 1 : 0);
					mCountLine = !mCountLine;
				} else {
					currentLine++;
				}
				VI_V_CURRENT.set<layouts::VI_V_CURRENT_Register::V_CURRENT>(currentLine);
			} else {
				mFirstLine = !mFirstLine;
			}


			{
				U16 vIntr = VI_V_INTR.get<layouts::VI_V_INTR_Register::V_INTR>();
				U16 vCurrent = VI_V_CURRENT.get<layouts::VI_V_CURRENT_Register::V_CURRENT>();
				BIT match = ESX_FALSE;
				if (VI_V_TOTAL.get<layouts::VI_V_TOTAL_Register::V_TOTAL>() & 0x1) {
					// Progressive: bit 0 ignored, match on [9:1]
					match = ((vIntr & ~0x1) == (vCurrent & ~0x1)) ? ESX_TRUE : ESX_FALSE;
				} else {
					// Interlaced
					if (vIntr & 0x1) {
						// V_INTR bit0=1: normal match on [9:1]
						match = ((vIntr & ~0x1) == (vCurrent & ~0x1)) ? ESX_TRUE : ESX_FALSE;
					} else {
						// V_INTR bit0=0: HW bug - odd field triggers one line early
						U16 vField = vCurrent & 0x1;
						if (vField == 0) {
							match = ((vIntr & ~0x1) == (vCurrent & ~0x1)) ? ESX_TRUE : ESX_FALSE;
						} else {
							match = ((vIntr == 0) ? (vCurrent >= (VI_V_TOTAL.get<layouts::VI_V_TOTAL_Register::V_TOTAL>() - 1))
								: (((vIntr & ~0x1) - 2) == (vCurrent & ~0x1))) ? ESX_TRUE : ESX_FALSE;
						}
					}
				}
				if (match == ESX_TRUE && mVIInterruptRaised == ESX_FALSE) {
					mRCP->setInterrupt(InterruptType::VI, ESX_FALSE, ESX_TRUE, 0);
					mVIInterruptRaised = ESX_TRUE;
				} else if (match == ESX_FALSE) {
					mVIInterruptRaised = ESX_FALSE;
				}
			}
		});

		Scheduler::AddSchedulerEventHandler(SchedulerEventType::GPUStartVBlank, [&](const SchedulerEvent& ev) {

		});

		Scheduler::AddSchedulerEventHandler(SchedulerEventType::GPUEndVBlank, [&](const SchedulerEvent& ev) {

		});

		Scheduler::AddSchedulerEventHandler(SchedulerEventType::GPUStartHBlank, [&](const SchedulerEvent& ev) {

		});

		Scheduler::AddSchedulerEventHandler(SchedulerEventType::GPUEndHBlank, [&](const SchedulerEvent& ev) {

		});
	}

	VideoInterface::~VideoInterface()
	{
	}

	void VideoInterface::init()
	{
	}

	void VideoInterface::clock(U64 clocks)
	{
	}

	void VideoInterface::store(U32 address, U32 value)
	{
		switch (address) {
			case 0x04400000: {
				U8 lastCtrl = VI_CTRL.get<layouts::VI_CTRL_Register::TYPE>();
				VI_CTRL.write(value);
				U8 newCtrl = VI_CTRL.get<layouts::VI_CTRL_Register::TYPE>();

				if (lastCtrl != newCtrl) {
					scheduleVIEvents();
				}
				break;
			}
			case 0x04400004: {
				ESX_CORE_LOG_INFO("VI_ORIGIN write: {:08x}h (was {:08x}h)", value, VI_ORIGIN.read());
				VI_ORIGIN.write(value);
				break;
			}
			case 0x04400008: {
				VI_WIDTH.write(value);
				break;
			}
			case 0x0440000C: {
				VI_V_INTR.write(value);
				break;
			}
			case 0x04400010: {
				mRCP->clearInterrupt(InterruptType::VI);
				break;
			}
			case 0x04400014: {
				VI_BURST.write(value);
				break;
			}
			case 0x04400018: {
				U32 lastValue = VI_V_TOTAL.read();
				VI_V_TOTAL.write(value);

				if(lastValue != VI_V_TOTAL.read()) scheduleVIEvents();
				break;
			}
			case 0x0440001C: {
				U32 lastValue = VI_H_TOTAL.read();
				VI_H_TOTAL.write(value);

				if (lastValue != VI_H_TOTAL.read()) scheduleVIEvents();
				break;
			}
			case 0x04400020: {
				U32 lastValue = VI_H_TOTAL_LEAP.read();
				VI_H_TOTAL_LEAP.write(value);

				if (lastValue != VI_H_TOTAL_LEAP.read()) scheduleVIEvents();
				break;
			}
			case 0x04400024: {
				U32 lastValue = VI_H_VIDEO.read();
				VI_H_VIDEO.write(value);

				if (lastValue != VI_H_VIDEO.read()) scheduleVIEvents();
				break;
			}
			case 0x04400028: {
				U32 lastValue = VI_V_VIDEO.read();
				VI_V_VIDEO.write(value);

				if (lastValue != VI_V_VIDEO.read()) scheduleVIEvents();
				break;
			}
			case 0x0440002C: {
				VI_V_BURST.write(value);
				break;
			}
			case 0x04400030: {
				VI_X_SCALE.write(value);
				break;
			}
			case 0x04400034: {
				VI_Y_SCALE.write(value);
				break;
			}
			case 0x04400038: {
				VI_TEST_ADDR.write(value);
				break;
			}
			case 0x0440003C: {
				VI_STAGED_DATA.write(value);
				break;
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	U32 VideoInterface::load(U32 address)
	{
		switch (address) {
			case 0x04400000: {
				return VI_CTRL.read();
			}
			case 0x04400004: {
				return VI_ORIGIN.read();
			}
			case 0x04400008: {
				return VI_WIDTH.read();
			}
			case 0x0440000C: {
				return VI_V_INTR.read();
			}
			case 0x04400010: {
				return VI_V_CURRENT.read();
			}
			case 0x04400014: {
				return VI_BURST.read();
			}
			case 0x04400018: {
				return VI_V_TOTAL.read();
			}
			case 0x0440001C: {
				return VI_H_TOTAL.read();
			}
			case 0x04400020: {
				return VI_H_TOTAL_LEAP.read();
			}
			case 0x04400024: {
				return VI_H_VIDEO.read();
			}
			case 0x04400028: {
				return VI_V_VIDEO.read();
			}
			case 0x0440002C: {
				return VI_V_BURST.read();
			}
			case 0x04400030: {
				return VI_X_SCALE.read();
			}
			case 0x04400034: {
				return VI_Y_SCALE.read();
			}
			case 0x04400038: {
				return VI_TEST_ADDR.read();
			}
			case 0x0440003C: {
				return VI_STAGED_DATA.read();
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Load from address 0x{:08x} not implemented yet", mName, address);
				break;
			}
		}
	return 0;
	}

	void VideoInterface::reset()
	{
	}

	inline U64 VideoInterface::VIClocksToCPUClocks(U64 VIClocks) {
		return (VIClocks * 93750000llu) / 48681812llu;
	}

	inline U64 VideoInterface::CPUClocksToVIClocks(U64 CPUClocks) {
		return (CPUClocks * 48681812llu) / 93750000llu;
	}

	void VideoInterface::scheduleVIEvents()
	{
		Scheduler::UnScheduleEventTypes({
			SchedulerEventType::GPUFrameStart,
			SchedulerEventType::GPUScanlineStart,
			SchedulerEventType::GPUStartVBlank,
			SchedulerEventType::GPUEndVBlank,
			SchedulerEventType::GPUStartHBlank,
			SchedulerEventType::GPUEndHBlank
		});

		if (VI_CTRL.get<layouts::VI_CTRL_Register::TYPE>() != 0) {
			U16 numScanlines = VI_V_TOTAL.get<layouts::VI_V_TOTAL_Register::V_TOTAL>();
			U16 scanlineLength = VI_H_TOTAL.get<layouts::VI_H_TOTAL_Register::H_TOTAL>();
			U16 startHsync = VI_H_VIDEO.get<layouts::VI_H_VIDEO_Register::H_START>() * 4;
			U16 endHsync = VI_H_VIDEO.get<layouts::VI_H_VIDEO_Register::H_END>() * 4;
			U16 startFrame = VI_V_VIDEO.get<layouts::VI_V_VIDEO_Register::V_START>();
			U16 endFrame = VI_V_VIDEO.get<layouts::VI_V_VIDEO_Register::V_END>();

			U64 cpuClocks = mRCP->getBus("Root")->getDevice<VR4300>("VR4300")->getClocks();

			SchedulerEvent frameStartEvent = {
				.Type = SchedulerEventType::GPUFrameStart,
				.ClockStart = cpuClocks,
				.ClockTarget = cpuClocks + 1,
				.Reschedule = ESX_TRUE,
				.RescheduleClocks = VIClocksToCPUClocks(scanlineLength * numScanlines)
			};

			SchedulerEvent scanlineStartEvent = {
				.Type = SchedulerEventType::GPUScanlineStart,
				.ClockStart = frameStartEvent.ClockStart,
				.ClockTarget = frameStartEvent.ClockTarget,
				.Reschedule = ESX_TRUE,
				.RescheduleClocks = VIClocksToCPUClocks(scanlineLength)
			};

			SchedulerEvent startHBlankEvent = {
				.Type = SchedulerEventType::GPUStartHBlank,
				.ClockStart = frameStartEvent.ClockStart,
				.ClockTarget = frameStartEvent.ClockTarget + VIClocksToCPUClocks(endHsync),
				.Reschedule = ESX_TRUE,
				.RescheduleClocks = VIClocksToCPUClocks(scanlineLength)
			};

			SchedulerEvent endHBlankEvent = {
				.Type = SchedulerEventType::GPUEndHBlank,
				.ClockStart = frameStartEvent.ClockStart,
				.ClockTarget = frameStartEvent.ClockTarget + VIClocksToCPUClocks(startHsync),
				.Reschedule = ESX_TRUE,
				.RescheduleClocks = VIClocksToCPUClocks(scanlineLength)
			};

			SchedulerEvent startVBlankEvent = {
				.Type = SchedulerEventType::GPUStartVBlank,
				.ClockStart = frameStartEvent.ClockStart,
				.ClockTarget = frameStartEvent.ClockTarget + VIClocksToCPUClocks(endFrame * scanlineLength),
				.Reschedule = ESX_TRUE,
				.RescheduleClocks = VIClocksToCPUClocks(scanlineLength * numScanlines)
			};

			SchedulerEvent endVBlankEvent = {
				.Type = SchedulerEventType::GPUEndVBlank,
				.ClockStart = frameStartEvent.ClockStart,
				.ClockTarget = frameStartEvent.ClockTarget + VIClocksToCPUClocks(startFrame * scanlineLength),
				.Reschedule = ESX_TRUE,
				.RescheduleClocks = VIClocksToCPUClocks(scanlineLength * numScanlines)
			};
			
			Scheduler::ScheduleEvent(frameStartEvent);
			Scheduler::ScheduleEvent(scanlineStartEvent);
			Scheduler::ScheduleEvent(startHBlankEvent);
			Scheduler::ScheduleEvent(endHBlankEvent);
			Scheduler::ScheduleEvent(startVBlankEvent);
			Scheduler::ScheduleEvent(endVBlankEvent);
		}
	}
}
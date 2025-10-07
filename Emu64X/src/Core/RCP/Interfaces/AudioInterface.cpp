#include "AudioInterface.h"

#include "../RCP.h"
#include "Core/MIPS/VR4300/VR4300.h"
#include "Core/Scheduler.h"

namespace esx {

	AudioInterface::AudioInterface(RCP* rcp)
		: mRCP(rcp)
	{
		Scheduler::AddSchedulerEventHandler(SchedulerEventType::AIDMAStart, [&](SchedulerEvent& ev) {
			mRCP->setInterrupt(InterruptType::AI, ESX_FALSE, ESX_TRUE, 0);
		});

		Scheduler::AddSchedulerEventHandler(SchedulerEventType::AIDMADone, [&](SchedulerEvent& ev) {
			if (AI_STATUS.get(layouts::AI_STATUS_Register::Field::FULL).as<BIT>() == ESX_TRUE) {
				AI_STATUS.set(layouts::AI_STATUS_Register::Field::FULL, ESX_FALSE);
			} else {
				AI_STATUS.set(layouts::AI_STATUS_Register::Field::BUSY, ESX_FALSE);
			}
		});
	}

	AudioInterface::~AudioInterface()
	{
	}

	void AudioInterface::init()
	{
	}

	void AudioInterface::clock(U64 clocks)
	{
	}

	void AudioInterface::store(U32 address, U32 value)
	{
		switch (address) {
			case 0x04500000: {
				AI_DRAM_ADDR.write(value);
				break;
			}
			case 0x04500004: {
				AI_LENGTH.write(value);


				if (AI_CONTROL.get(layouts::AI_CONTROL_Register::Field::DMA_ENABLE).as<BIT>() == ESX_TRUE && AI_STATUS.get(layouts::AI_STATUS_Register::Field::FULL).as<BIT>() == ESX_FALSE) {
					BIT DMAAlreadyUp = AI_STATUS.get(layouts::AI_STATUS_Register::Field::BUSY).as<BIT>();
					AI_STATUS.set(layouts::AI_STATUS_Register::Field::FULL, DMAAlreadyUp);
					AI_STATUS.set(layouts::AI_STATUS_Register::Field::BUSY, ESX_TRUE);

					U64 cpuClocks = mRCP->getBus("Root")->getDevice<VR4300>("VR4300")->getClocks();

					U64 TargetClock = DMAAlreadyUp ? Scheduler::NextEventOfType(SchedulerEventType::AIDMADone).value()->ClockTarget : cpuClocks;
					U64 Length = AI_LENGTH.get(layouts::AI_LENGTH_Register::Field::LENGTH).as<U64>();
					U64 DACRate = AI_DACRATE.get(layouts::AI_DACRATE_Register::Field::DACRATE).as<U64>();
					U64 SampleClock = (((Length / 4llu) * (DACRate + 1)) * 93750000llu) / 48681812llu;

					SchedulerEvent dmaStartEvent = {
							.Type = SchedulerEventType::AIDMAStart,
							.ClockStart = cpuClocks,
							.ClockTarget = TargetClock + 1
					};

					SchedulerEvent dmaDoneEvent = {
							.Type = SchedulerEventType::AIDMADone,
							.ClockStart = cpuClocks,
							.ClockTarget = TargetClock + SampleClock
					};
					dmaDoneEvent.Write(AI_DRAM_ADDR.read());
					dmaDoneEvent.Write(AI_LENGTH.read());

					Scheduler::ScheduleEvent(dmaStartEvent);
					Scheduler::ScheduleEvent(dmaDoneEvent);
				}

				break;
			}
			case 0x04500008: {
				AI_CONTROL.write(value);

				AI_STATUS.set(layouts::AI_STATUS_Register::Field::ENABLED, AI_CONTROL.get(layouts::AI_CONTROL_Register::Field::DMA_ENABLE).as<BIT>());
				break;
			}
			case 0x0450000C: {
				mRCP->clearInterrupt(InterruptType::AI);
				break;
			}
			case 0x04500010: {
				AI_DACRATE.write(value);
				break;
			}
			case 0x04500014: {
				AI_BITRATE.write(value);
				break;
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	U32 AudioInterface::load(U32 address)
	{
		switch (address) {
			case 0x04500000: {
				return AI_DRAM_ADDR.read();
			}
			case 0x04500004:
			case 0x04500008:
			case 0x04500010:
			case 0x04500014: {
				return AI_LENGTH.read();
			}

			case 0x0450000C: {
				return AI_STATUS.read();
				break;
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Load from address 0x{:08x} not implemented yet", mName, address);
				break;
			}
		}
	}

	void AudioInterface::reset()
	{
	}

}
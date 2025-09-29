#include "SerialInterface.h"

#include "../RCP.h"
#include "Core/RDRAM.h"
#include "Core/SIExternalBus.h"

#include "Core/MIPS/VR4300/VR4300.h"
#include "Core/Scheduler.h"

namespace esx {

	SerialInterface::SerialInterface(RCP* rcp)
		: mRCP(rcp)
	{
		Scheduler::AddSchedulerEventHandler(SchedulerEventType::SIDMADone, [&](SchedulerEvent& ev) {
			BIT Write = ev.Read<BIT>();
			U32 PIFAddr = 0x1FC00000 + ev.Read<U32>();

			if (Write == ESX_FALSE) {
				mSIExtBus->ExecuteJoybusFrame();
			}

			U32 RDRAMBaseAddr = SI_DRAM_ADDR.read();
			for (U32 RDRAMOffset = 0; RDRAMOffset < 64; RDRAMOffset += 4) {
				U32 RDRAMAddr = RDRAMBaseAddr + RDRAMOffset;

				if (ADDRESS_UNALIGNED(RDRAMAddr, U32)) {
					SI_STATUS.set(layouts::SI_STATUS_Register::Field::DMA_ERROR, ESX_TRUE);
					break;
				}

				if (Write == ESX_TRUE) {
					U32 RDRAMOutput = 0;
					mRDRAM->load("Root", RDRAMAddr, RDRAMOutput, 0, 32);
					mSIExtBus->store("Root", PIFAddr, RDRAMOutput, 0, 32);
				} else {
					U32 PIFOutput = 0;
					mSIExtBus->load("Root", PIFAddr, PIFOutput, 0, 32);
					mRDRAM->store("Root", RDRAMAddr, PIFOutput, 0, 32);
				}

				SI_DRAM_ADDR.write(RDRAMAddr);
				PIFAddr += 4;
			}

			mRCP->setInterrupt(InterruptType::SI, ESX_FALSE, ESX_TRUE, 0);
			SI_STATUS.set(layouts::SI_STATUS_Register::Field::IO_BUSY, ESX_FALSE);
			SI_STATUS.set(layouts::SI_STATUS_Register::Field::DMA_BUSY, ESX_FALSE);
			SI_STATUS.set(layouts::SI_STATUS_Register::Field::INTERRUPT, ESX_TRUE);
			SI_STATUS.set(layouts::SI_STATUS_Register::Field::DMA_STATE, 0);
			SI_STATUS.set(layouts::SI_STATUS_Register::Field::PCH_STATE, 0);
		});
	}

	SerialInterface::~SerialInterface()
	{
	}

	void SerialInterface::init()
	{
		mRDRAM = mRCP->getBus("Root")->getDevice<RDRAM>("RDRAM");
		mSIExtBus = mRCP->getBus("Root")->getDevice<SIExternalBus>("SIExternalBus");
	}

	void SerialInterface::clock(U64 clocks)
	{
	}

	void SerialInterface::store(U32 address, U32 value)
	{
		switch (address) {
			case 0x04800000: {
				SI_DRAM_ADDR.write(value);
				break;
			}

			case 0x04800004: {
				SI_PIF_AD_RD64B.write(value);

				SI_STATUS.set(layouts::SI_STATUS_Register::Field::IO_BUSY, ESX_TRUE);
				SI_STATUS.set(layouts::SI_STATUS_Register::Field::DMA_BUSY, ESX_TRUE);
				SI_STATUS.set(layouts::SI_STATUS_Register::Field::DMA_STATE, 1);
				SI_STATUS.set(layouts::SI_STATUS_Register::Field::PCH_STATE, 1);

				if (Scheduler::NextEventOfType(SchedulerEventType::SIDMADone).has_value()) {
					SI_STATUS.set(layouts::SI_STATUS_Register::Field::DMA_ERROR, ESX_TRUE);
				}
				else {
					U64 cpuClocks = mRCP->getBus("Root")->getDevice<VR4300>("VR4300")->getClocks();

					SchedulerEvent dmaDoneEvent = {
							.Type = SchedulerEventType::SIDMADone,
							.ClockStart = cpuClocks,
							.ClockTarget = cpuClocks + 1
					};
					dmaDoneEvent.Write(ESX_FALSE);
					dmaDoneEvent.Write(SI_PIF_AD_RD64B.read());

					Scheduler::ScheduleEvent(dmaDoneEvent);
				}
				break;
			}

			case 0x04800010: {
				SI_PIF_AD_WR64B.write(value);

				SI_STATUS.set(layouts::SI_STATUS_Register::Field::IO_BUSY, ESX_TRUE);
				SI_STATUS.set(layouts::SI_STATUS_Register::Field::DMA_BUSY, ESX_TRUE);
				SI_STATUS.set(layouts::SI_STATUS_Register::Field::DMA_STATE, 1);
				SI_STATUS.set(layouts::SI_STATUS_Register::Field::PCH_STATE, 1);

				if (Scheduler::NextEventOfType(SchedulerEventType::SIDMADone).has_value()) {
					SI_STATUS.set(layouts::SI_STATUS_Register::Field::DMA_ERROR, ESX_TRUE);
				} else {
					U64 cpuClocks = mRCP->getBus("Root")->getDevice<VR4300>("VR4300")->getClocks();

					SchedulerEvent dmaDoneEvent = {
							.Type = SchedulerEventType::SIDMADone,
							.ClockStart = cpuClocks,
							.ClockTarget = cpuClocks + 1
					};
					dmaDoneEvent.Write(ESX_TRUE);
					dmaDoneEvent.Write(SI_PIF_AD_WR64B.read());

					Scheduler::ScheduleEvent(dmaDoneEvent);
				}
				break;
			}

			case 0x04800018: {
				SI_STATUS.set(layouts::SI_STATUS_Register::Field::INTERRUPT, ESX_FALSE);
				mRCP->clearInterrupt(InterruptType::SI);
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	U32 SerialInterface::load(U32 address)
	{
		switch (address) {
			case 0x04800000: {
				return SI_DRAM_ADDR.read();
			}
			case 0x04800004: {
				return SI_PIF_AD_RD64B.read();
			}
			case 0x04800010: {
				return SI_PIF_AD_WR64B.read();
			}
			case 0x04800018: {
				return SI_STATUS.read();
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Load from address 0x{:08x} not implemented yet", mName, address);
				break;
			}
		}
		return 0;
	}

	void SerialInterface::reset()
	{
	}
}
#include "PeripheralInterface.h"

#include "../RCP.h"
#include "Core/RDRAM.h"
#include "Core/PIExternalBus.h"

#include "Core/MIPS/VR4300/VR4300.h"
#include "Core/Scheduler.h"

namespace esx {

	PeripheralInterface::PeripheralInterface(RCP* rcp)
		: mRCP(rcp)
	{
		Scheduler::AddSchedulerEventHandler(SchedulerEventType::PIDMADone, [&](const SchedulerEvent& ev) {
			size_t RDRAMPageSize = 0x800;

			U32 DRAM_Address = PI_DRAM_ADDR.get<layouts::PI_DRAM_ADDR_Register::DRAM_ADDR>() << 1;
			U32 CART_Address = PI_CART_ADDR.get<layouts::PI_CART_ADDR_Register::CART_ADDR>() << 1;
			I32 Length = (I32)PI_WR_LEN.get<layouts::PI_WR_LEN_Register::WR_LEN>() + 1;

			I32 RemainingLength = Length;
			U32 StartMisalignment = DRAM_Address & 0x7;
			U32 Misalignment = StartMisalignment;
			BIT FirstBlock = ESX_TRUE;
			U8 Buffer[128];

			fseek(mPIExtBus->mCartridge, CART_Address - 0x10000000, SEEK_SET);
			while (RemainingLength > 0) {
				U32 PageEnd = RDRAMPageSize - (DRAM_Address % RDRAMPageSize);

				//if misalignment is 6, the maximum size is not 128 but 122, because the first 6 bytes are skipped.
				U32 BlockSize = std::min<U32>({ U32(RemainingLength), PageEnd, (128 - Misalignment) });

				if (FirstBlock == ESX_FALSE) {
					// All PI accesses are always 16-bit long, so if the block size was odd (which happens on the last block, if the remaining length is odd), 
					// one extra byte will be fetched from PI into the internal buffer.
					if ((BlockSize % 2) != 0) 
						BlockSize++;
				} else {
					// Writes to RDRAM seems to use some kind of masking, so they are correctly done at the byte granularity.
					// This means that odd length transfers in the first block appear to work correctly. 
					// Notice that this applies only to the first block whatever its size is; the size (as described above) might be limited by the end of the RDRAM page, 
					// in which case only odd transfers up to there are working correctly.
					if(BlockSize == (128 - Misalignment - 1)) BlockSize++;
				}

				// The internal 128 byte buffer is filled starting from the index matching the misalignment. 
				// This might affect the maximum size of the first block: for instance, if misalignment is 6, the maximum size is not 128 but 122, because the first 6 bytes are skipped.
				fread_s(&Buffer[Misalignment], sizeof(Buffer), sizeof(U8), BlockSize, mPIExtBus->mCartridge);
				memcpy(&mRDRAM->mMemory[DRAM_Address], &Buffer[Misalignment], BlockSize - Misalignment);

				CART_Address += BlockSize;
				DRAM_Address += BlockSize;

				//RDRAM address register is always rounded up to the next 8 byte alignment at the end of the first block. 
				//In most normal cases, the logic above already ensures that the address ends up being aligned at the end of the block, 
				// but the rounding up happens even in cases like short transfers that ends with the first block at ends at an arbitrary byte.
				DRAM_Address &= ~0x7; 

				RemainingLength -= BlockSize;
				Misalignment = 0;
				FirstBlock = ESX_FALSE;
			}
			
			mRCP->setInterrupt(InterruptType::PI, ESX_FALSE, ESX_TRUE, 0);
			PI_STATUS.set<layouts::PI_STATUS_Register::DMA_BUSY>(ESX_FALSE);
			PI_STATUS.set<layouts::PI_STATUS_Register::DMA_COMPLETED>(ESX_TRUE);
			PI_DRAM_ADDR.set<layouts::PI_DRAM_ADDR_Register::DRAM_ADDR>(DRAM_Address >> 1);
			PI_CART_ADDR.set<layouts::PI_CART_ADDR_Register::CART_ADDR>(CART_Address >> 1);
			mLastMisalignment = (Length < 8) ? StartMisalignment : 0;
		});
	}

	PeripheralInterface::~PeripheralInterface()
	{
	}

	void PeripheralInterface::init()
	{
		mRDRAM = mRCP->getBus("Root")->getDevice<RDRAM>("RDRAM");
		mPIExtBus = mRCP->getBus("Root")->getDevice<PIExternalBus>("PIExternalBus");
	}

	void PeripheralInterface::clock(U64 clocks)
	{
	}

	void PeripheralInterface::store(U32 address, U32 value)
	{
		switch (address) {
			case 0x04600000: {
				PI_DRAM_ADDR.write(value);
				break;
			}
			case 0x04600004: {
				PI_CART_ADDR.write(value);
				break;
			}
			case 0x04600008: {
				PI_RD_LEN.write(value);

				ESX_CORE_LOG_WARNING("{} - DMA From RDRAM to PI not implemented yet");
				break;
			}
			case 0x0460000C: {
				PI_WR_LEN.write(value);
				
				startDMAToRDRAM();
				break;
			}
			case 0x04600010: {
				PI_STATUS_Write_Register writeReg;
				writeReg.write(value);

				if (writeReg.get<layouts::PI_STATUS_Write_Register::RESET_DMA>()) {
					//Reset DMA Controller
					Scheduler::UnScheduleAllEvents(SchedulerEventType::PIDMADone);
					PI_STATUS.set<layouts::PI_STATUS_Register::IO_BUSY>(ESX_FALSE);
					PI_STATUS.set<layouts::PI_STATUS_Register::DMA_ERROR>(ESX_FALSE);
					PI_STATUS.set<layouts::PI_STATUS_Register::DMA_BUSY>(ESX_FALSE);
					PI_STATUS.set<layouts::PI_STATUS_Register::DMA_COMPLETED>(ESX_FALSE);
				}

				if (writeReg.get<layouts::PI_STATUS_Write_Register::CLEAR_INTERRUPT>()) {
					//Clear interrupt
					mRCP->clearInterrupt(InterruptType::PI);
					PI_STATUS.set<layouts::PI_STATUS_Register::DMA_COMPLETED>(ESX_FALSE);
				}


				break;
			}
			case 0x04600014: {
				PI_BSD_DOM1_LAT.write(value);
				break;
			}
			case 0x04600018: {
				PI_BSD_DOM1_PWD.write(value);
				break;
			}
			case 0x0460001C: {
				PI_BSD_DOM1_PGS.write(value);
				break;
			}
			case 0x04600020: {
				PI_BSD_DOM1_RLS.write(value);
				break;
			}
			case 0x04600024: {
				PI_BSD_DOM2_LAT.write(value);
				break;
			}
			case 0x04600028: {
				PI_BSD_DOM2_PWD.write(value);
				break;
			}
			case 0x0460002C: {
				PI_BSD_DOM2_PGS.write(value);
				break;
			}
			case 0x04600030: {
				PI_BSD_DOM2_RLS.write(value);
				break;
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	U32 PeripheralInterface::load(U32 address)
	{
		switch (address) {
			case 0x04600000: {
				return PI_DRAM_ADDR.read();
				break;
			}
			case 0x04600004: {
				return PI_CART_ADDR.read();
				break;
			}
			case 0x04600008: {
				return 0x7F - mLastMisalignment;
			}
			case 0x0460000C: {
				return 0x7F - mLastMisalignment;
			}
			case 0x04600010: {
				return PI_STATUS.read();
				break;
			}
			case 0x04600014: {
				return PI_BSD_DOM1_LAT.read();
				break;
			}
			case 0x04600018: {
				return PI_BSD_DOM1_PWD.read();
				break;
			}
			case 0x0460001C: {
				return PI_BSD_DOM1_PGS.read();
				break;
			}
			case 0x04600020: {
				return PI_BSD_DOM1_RLS.read();
				break;
			}
			case 0x04600024: {
				return PI_BSD_DOM2_LAT.read();
				break;
			}
			case 0x04600028: {
				return PI_BSD_DOM2_PWD.read();
				break;
			}
			case 0x0460002C: {
				return PI_BSD_DOM2_PGS.read();
				break;
			}
			case 0x04600030: {
				return PI_BSD_DOM2_RLS.read();
				break;
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Load from address 0x{:08x} not implemented yet", mName, address);
				break;
			}
		}
	return 0;
	}

	void PeripheralInterface::reset()
	{
		mLastMisalignment = 0;
	}

	void PeripheralInterface::startDMAToRDRAM()
	{
		U32 Length = PI_WR_LEN.get<layouts::PI_WR_LEN_Register::WR_LEN>() + 1;
		U64 cpuClocks = mRCP->getBus("Root")->getDevice<VR4300>("VR4300")->getClocks();

		SchedulerEvent dmaDoneEvent = {
				.Type = SchedulerEventType::PIDMADone,
				.ClockStart = cpuClocks,
				.ClockTarget = cpuClocks + RCP::RCPClocksToCPUClocks((Length / 2) * (PI_BSD_DOM1_RLS.get<layouts::PI_BSD_DOM_RLS_Register::RLS>() + 1))
		};

		Scheduler::ScheduleEvent(dmaDoneEvent);

		PI_STATUS.set<layouts::PI_STATUS_Register::DMA_BUSY>(ESX_TRUE);
		PI_STATUS.set<layouts::PI_STATUS_Register::DMA_COMPLETED>(ESX_FALSE);
	}

}
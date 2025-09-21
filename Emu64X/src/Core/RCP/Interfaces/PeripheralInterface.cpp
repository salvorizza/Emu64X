#include "PeripheralInterface.h"

#include "../RCP.h"
#include "Core/RDRAM.h"
#include "Core/PIExternalBus.h"

namespace esx {

	PeripheralInterface::PeripheralInterface(RCP* rcp)
		: mRCP(rcp)
	{
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
				//Start DMA from RDRAM to PI
				break;
			}
			case 0x0460000C: {
				PI_WR_LEN.write(value);
				
				startDMAToRDRAM();
				//Start DMA from PI to RDRAM
				break;
			}
			case 0x04600010: {
				PI_STATUS_Write_Register writeReg;
				writeReg.write(value);

				if (writeReg.get(layouts::PI_STATUS_Write_Register::Field::RESET_DMA).as<BIT>()) {
					//Reset DMA Controller
					//TODO: Stop transfers
					PI_STATUS.set(layouts::PI_STATUS_Register::Field::IO_BUSY, ESX_FALSE);
					PI_STATUS.set(layouts::PI_STATUS_Register::Field::DMA_ERROR, ESX_FALSE);
					PI_STATUS.set(layouts::PI_STATUS_Register::Field::DMA_BUSY, ESX_FALSE);
				}

				if (writeReg.get(layouts::PI_STATUS_Write_Register::Field::CLEAR_INTERRUPT).as<BIT>()) {
					//Clear interrupt
					mRCP->clearInterrupt(InterruptType::PI);
					PI_STATUS.set(layouts::PI_STATUS_Register::Field::DMA_COMPLETED, ESX_FALSE);
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
				return 0x7F;
			}
			case 0x0460000C: {
				return 0x7F;
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
	}

	void PeripheralInterface::reset()
	{
	}

	void PeripheralInterface::startDMAToRDRAM()
	{
		U32 DRAM_Address = PI_DRAM_ADDR.get(layouts::PI_DRAM_ADDR_Register::Field::DRAM_ADDR).as<U32>() << 1;
		U32 CART_Address = PI_CART_ADDR.get(layouts::PI_CART_ADDR_Register::Field::CART_ADDR).as<U32>() << 1;
		U32 Length = PI_WR_LEN.get(layouts::PI_WR_LEN_Register::Field::WR_LEN).as<U32>() + 1;
		U8 Buffer[128];

		if (CART_Address >= 0x10000000) {
			fseek(mPIExtBus->mCartridge, CART_Address - 0x10000000, SEEK_SET);
			fread_s(&mRDRAM->mMemory[DRAM_Address], Length, Length, 1, mPIExtBus->mCartridge);
		}

		mRCP->setInterrupt(InterruptType::PI, PI_STATUS.get(layouts::PI_STATUS_Register::Field::DMA_COMPLETED).as<BIT>(), ESX_TRUE, 0);
		PI_STATUS.set(layouts::PI_STATUS_Register::Field::DMA_COMPLETED, ESX_TRUE);
	}

}
#include "PeripheralInterface.h"

#include "../RCP.h"

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
	}

	void PeripheralInterface::clock(U64 clocks)
	{
	}

	void PeripheralInterface::store(U32 address, U32 value)
	{
		switch (address) {
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

}
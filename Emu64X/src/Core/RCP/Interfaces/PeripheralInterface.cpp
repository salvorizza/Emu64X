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

				if (writeReg.get(PI_STATUS_Write_RegisterLayout::Field::RESET_DMA).as<BIT>()) {
					//Reset DMA Controller
					//TODO: Stop transfers
					mPI_STATUS.set(PI_STATUS_RegisterLayout::Field::IO_BUSY, ESX_FALSE);
					mPI_STATUS.set(PI_STATUS_RegisterLayout::Field::DMA_ERROR, ESX_FALSE);
					mPI_STATUS.set(PI_STATUS_RegisterLayout::Field::DMA_BUSY, ESX_FALSE);
				}

				if (writeReg.get(PI_STATUS_Write_RegisterLayout::Field::CLEAR_INTERRUPT).as<BIT>()) {
					//Clear interrupt
					mRCP->clearInterrupt(InterruptType::PI);
					mPI_STATUS.set(PI_STATUS_RegisterLayout::Field::DMA_COMPLETED, ESX_FALSE);
				}


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
				return mPI_STATUS.read();
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
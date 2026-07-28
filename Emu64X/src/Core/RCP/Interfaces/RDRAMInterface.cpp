#include "RDRAMInterface.h"

#include "Core/MIPS/VR4300/VR4300.h"

namespace esx {

	RDRAMInterface::RDRAMInterface(RCP* rcp)
	{
	}

	RDRAMInterface::~RDRAMInterface()
	{
	}

	void RDRAMInterface::init()
	{
	}

	void RDRAMInterface::clock(U64 clocks)
	{
	}

	void RDRAMInterface::store(U32 address, U32 value)
	{
		switch (address) {
			case 0x04700000: {
				RI_MODE.write(value);
				break;
			}
			case 0x04700004: {
				RI_CONFIG.write(value);
				break;
			}
			case 0x04700008: {
				//TODO:
				//Any write to this register causes a new value to be loaded into the RAC current control register. Corresponds to the RAC CCtlLd input signal.
				//The value loaded depends on the contents of the RI_CONFIG register, see there for details.
				//TOVERIFY: When AutoCC = 1 in RI_CONFIG and this register is written, a sufficient delay should be observed to let CC autocalibration stabilize.
				break;
			}
			case 0x0470000C: {
				RI_SELECT.write(value);
				break;
			}
			case 0x04700010: {
				RI_REFRESH.write(value);
				break;
			}
			case 0x04700014: {
				RI_LATENCY.write(value);
				break;
			}
			case 0x04700018: {
				ESX_CORE_LOG_WARNING("{} - RI_ERROR Is read-only", mName);
				break;
			}
			case 0x0470001C: {
				ESX_CORE_LOG_WARNING("{} - RI_BANK_STATUS Is read-only", mName);
				break;
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	U32 RDRAMInterface::load(U32 address)
	{
		switch (address) {
			case 0x04700000: {
				return RI_MODE.read();
			}
			case 0x04700004: {
				return RI_CONFIG.read();
			}
			case 0x04700008: {
				RI_CURRENT_LOAD_Register reg;
				reg.write(0b00110);
				reg.set<layouts::RI_CURRENT_LOAD_Register::Ack>(RI_ERROR.get<layouts::RI_ERROR_Register::Ack>());
				reg.set<layouts::RI_CURRENT_LOAD_Register::STOP_R>(RI_MODE.get<layouts::RI_MODE_Register::STOP_R>());
				reg.set<layouts::RI_CURRENT_LOAD_Register::TSEL>(RI_SELECT.get<layouts::RI_SELECT_Register::TSEL>() & 0x1);

				return reg.read();
			}
			case 0x0470000C: {
				return RI_SELECT.read();
			}
			case 0x04700010: {
				return RI_REFRESH.read();
			}
			case 0x04700014: {
				return RI_LATENCY.read();
			}
			case 0x04700018: {
				return RI_ERROR.read();
			}
			case 0x0470001C: {
				return RI_BANK_STATUS.read();
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Load from address 0x{:08x} not implemented yet", mName, address);
				break;
			}
		}

		return 0;
	}

	void RDRAMInterface::reset()
	{
	}
}
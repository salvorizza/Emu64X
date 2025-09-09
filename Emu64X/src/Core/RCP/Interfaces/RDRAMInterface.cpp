#include "RDRAMInterface.h"

#include "Core/MIPS/VR4300/VR4300.h"

namespace esx {

	RDRAMInterface::RDRAMInterface(RCP* rcp)
		: mRCP(rcp)
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
			default: {
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	U32 RDRAMInterface::load(U32 address)
	{
		switch (address) {
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
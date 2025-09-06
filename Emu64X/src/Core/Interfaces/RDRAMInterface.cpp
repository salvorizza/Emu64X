#include "RDRAMInterface.h"

#include "Core/MIPS/VR4300/VR4300.h"

namespace esx {

	RDRAMInterface::RDRAMInterface()
		: BusDevice(ESX_TEXT("RDRAMInterface"))
	{
		addRange(ESX_TEXT("Root"), 0x04700000, 0x000FFFFF, 0xFFFFFFFF);

		reset();
	}

	RDRAMInterface::~RDRAMInterface()
	{
	}

	void RDRAMInterface::clock(U64 clocks)
	{
	}

	void RDRAMInterface::store(const StringView& busName, U32 address, U32 value)
	{
		switch (address) {
			default: {
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	void RDRAMInterface::load(const StringView& busName, U32 address, U32& output)
	{
		switch (address) {
			default: {
				ESX_CORE_LOG_WARNING("{} - Load from address 0x{:08x} not implemented yet", mName, address);
				break;
			}
		}
	}

	void RDRAMInterface::reset()
	{
	}
}
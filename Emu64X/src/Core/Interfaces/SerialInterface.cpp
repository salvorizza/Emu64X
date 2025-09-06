#include "SerialInterface.h"

#include "Core/MIPS/VR4300/VR4300.h"

namespace esx {

	SerialInterface::SerialInterface()
		: BusDevice(ESX_TEXT("SerialInterface"))
	{
		addRange(ESX_TEXT("Root"), 0x04800000, 0x000FFFFF, 0xFFFFFFFF);

		reset();
	}

	SerialInterface::~SerialInterface()
	{
	}

	void SerialInterface::clock(U64 clocks)
	{
	}

	void SerialInterface::store(const StringView& busName, U32 address, U32 value)
	{
		switch (address) {
			default: {
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	void SerialInterface::load(const StringView& busName, U32 address, U32& output)
	{
		switch (address) {
			default: {
				ESX_CORE_LOG_WARNING("{} - Load from address 0x{:08x} not implemented yet", mName, address);
				break;
			}
		}
	}

	void SerialInterface::reset()
	{
	}
}
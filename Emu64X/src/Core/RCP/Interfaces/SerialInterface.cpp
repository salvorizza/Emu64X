#include "SerialInterface.h"

namespace esx {

	SerialInterface::SerialInterface(RCP* rcp)
		: mRCP(rcp)
	{
	}

	SerialInterface::~SerialInterface()
	{
	}

	void SerialInterface::init()
	{
	}

	void SerialInterface::clock(U64 clocks)
	{
	}

	void SerialInterface::store(U32 address, U32 value)
	{
		switch (address) {
			default: {
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	U32 SerialInterface::load(U32 address)
	{
		switch (address) {
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
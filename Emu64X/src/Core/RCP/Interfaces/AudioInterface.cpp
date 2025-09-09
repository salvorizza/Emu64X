#include "AudioInterface.h"

#include "../RCP.h"

namespace esx {

	AudioInterface::AudioInterface(RCP* rcp)
		: mRCP(rcp)
	{
	}

	AudioInterface::~AudioInterface()
	{
	}

	void AudioInterface::init()
	{
	}

	void AudioInterface::clock(U64 clocks)
	{
	}

	void AudioInterface::store(U32 address, U32 value)
	{
		switch (address) {
			case 0x04500000: {
				AI_DRAM_ADDR.write(value);
				break;
			}
			case 0x04500004: {
				AI_LENGTH.write(value);
				break;
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	U32 AudioInterface::load(U32 address)
	{
		switch (address) {
			case 0x04500000: {
				return AI_DRAM_ADDR.read();
				break;
			}
			case 0x04500004: {
				return AI_LENGTH.read();
				break;
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Load from address 0x{:08x} not implemented yet", mName, address);
				break;
			}
		}
	}

	void AudioInterface::reset()
	{
	}

}
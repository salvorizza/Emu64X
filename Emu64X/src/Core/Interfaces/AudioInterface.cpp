#include "AudioInterface.h"

#include "Core/MIPS/VR4300/VR4300.h"

namespace esx {

	AudioInterface::AudioInterface()
		: BusDevice(ESX_TEXT("AudioInterface"))
	{
		addRange(ESX_TEXT("Root"), 0x04500000, 0x000FFFFF, 0xFFFFFFFF);

		reset();
	}

	AudioInterface::~AudioInterface()
	{
	}

	void AudioInterface::clock(U64 clocks)
	{
	}

	void AudioInterface::store(const StringView& busName, U32 address, U32 value)
	{
		switch (address) {
			default: {
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	void AudioInterface::load(const StringView& busName, U32 address, U32& output)
	{
		switch (address) {
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
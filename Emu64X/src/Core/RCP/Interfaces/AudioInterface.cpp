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
			case 0x04500008: {
				AI_CONTROL.write(value);

				if (AI_CONTROL.get(layouts::AI_CONTROL_Register::Field::DMA_ENABLE).as<BIT>() == ESX_TRUE) {
					//Start DMA
					ESX_CORE_LOG_WARNING("{} - DMA not implemented yet", mName);
				}
				break;
			}
			case 0x0450000C: {
				mRCP->clearInterrupt(InterruptType::AI);
				break;
			}
			case 0x04500010: {
				AI_DACRATE.write(value);
				break;
			}
			case 0x04500014: {
				AI_BITRATE.write(value);
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
			}
			case 0x04500004:
			case 0x04500008:
			case 0x04500010:
			case 0x04500014: {
				return AI_LENGTH.read();
			}

			case 0x0450000C: {
				return AI_STATUS.read();
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
#include "RDRAM.h"

namespace esx {



	RDRAM::RDRAM()
		: BusDevice("RDRAM")
	{
		reset();

		addRange(ESX_TEXT("Root"), 0x00000000, 0x03FFFFFF, 0xFFFFFFFF);
	}

	RDRAM::~RDRAM()
	{
	}

	void RDRAM::init()
	{
		mMemory.resize(MIBI(4));
	}

	void RDRAM::store(const StringView& busName, U32 address, U32 value)
	{
		if (address >= 0x00000000 && address <= 0x03EFFFFF) {
			if (address < mMemory.size()) {
				*reinterpret_cast<U32*>(&mMemory[address]) = _byteswap_ulong(value);
			}
		}
		else if (address >= 0x03F00000 && address <= 0x03FFFFFF) {
			//TODO: RDRAM Registers + RDRAM Registers (broadcast)
		}
	}

	void RDRAM::load(const StringView& busName, U32 address, U32& output)
	{
		output = 0;
		if (address >= 0x00000000 && address <= 0x03EFFFFF) {
			if (address < mMemory.size()) {
				output = _byteswap_ulong(*reinterpret_cast<U32*>(&mMemory[address]));
			}
		}
		else if (address >= 0x03F00000 && address <= 0x03F7FFFF) {
			//TODO: RDRAM Registers
		}
		else if (address >= 0x03F80000 && address <= 0x03FFFFFF) {
			//RDRAM Registers (broadcast) write-only
		}
	}

	void RDRAM::reset() {
		std::fill(mMemory.begin(), mMemory.end(), 0x00);
	}
}
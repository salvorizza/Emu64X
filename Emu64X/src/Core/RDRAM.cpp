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

	void RDRAM::store(const StringView& busName, U32 address, U32 value, U8 lowerBits, U8 accessSize)
	{
		U32 mask = generateMask(lowerBits, accessSize);

		if (address >= 0x00000000 && address <= 0x03EFFFFF) {
			if (address < mMemory.size()) {
				U32* ptr = reinterpret_cast<U32*>(&mMemory[address]);
				U32 temp = _byteswap_ulong(*ptr);
				*ptr = _byteswap_ulong((value & mask) | (temp & ~mask));
			}
		}
		else if (address >= 0x03F00000 && address <= 0x03FFFFFF) {
			//TODO: RDRAM Registers + RDRAM Registers (broadcast)
		}
	}

	void RDRAM::load(const StringView& busName, U32 address, U32& output, U8 lowerBits, U8 accessSize)
	{
		U32 mask = generateMask(lowerBits, accessSize);

		output = 0;
		if (address >= 0x00000000 && address <= 0x03EFFFFF) {
			if (address < mMemory.size()) {
				output = _byteswap_ulong(*reinterpret_cast<U32*>(&mMemory[address])) & mask;
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

	inline U32 RDRAM::generateMask(U8 lowerBits, U8 accessSize) const
	{
		U32 mask = 0;

		switch (accessSize) {
			case 8:
				mask = 0xFFu << (lowerBits * 8);
				break;
			case 16:
				mask = 0xFFFFu << (lowerBits * 8);
				break;
			default:
				mask = 0xFFFFFFFFu;
				break;
		}

		return mask;
	}
}
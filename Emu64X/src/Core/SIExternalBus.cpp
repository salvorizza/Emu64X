#include "SIExternalBus.h"

#include <intrin.h>

namespace esx {

	SIExternalBus::SIExternalBus(StringView path)
		:	BusDevice(ESX_TEXT("SIExternalBus")),
			mPIF_Path(path)
	{
		addRange(ESX_TEXT("Root"), 0x1FC00000, 0xFFFFF, 0xFFFFFFFF);

		reset();

	}

	SIExternalBus::~SIExternalBus()
	{
	}

	void SIExternalBus::init()
	{
		std::ifstream input(mPIF_Path.data(), std::ios::binary);
		mPIF_ROM.insert(mPIF_ROM.begin(), std::istreambuf_iterator<char>(input), {});
		mPIF_ROM.resize(KIBI(2));
		input.close();

		mPIF_RAM.resize(0x40);
	}

	void SIExternalBus::load(const StringView& busName, U32 address, U32& output)
	{
		if (address >= 0x1FC00000 && address <= 0x1FC007BF) {
			output = _byteswap_ulong(*reinterpret_cast<U32*>(&mPIF_ROM[address - 0x1FC00000]));
		}
		else if (address >= 0x1FC007C0 && address <= 0x1FC007FF) {
			output = *reinterpret_cast<U32*>(&mPIF_RAM[address - 0x1FC007C0]);
		}
		else if (address >= 0x1FC00800 && address <= 0x1FCFFFFF) {
			//Reserved
		}
	}


	void SIExternalBus::reset()
	{
		std::fill(mPIF_RAM.begin(), mPIF_RAM.end(), 0);
	}

}
#include "PIExternalBus.h"

#include <intrin.h>

namespace esx {



	PIExternalBus::PIExternalBus()
		:	BusDevice(ESX_TEXT("PIExternalBus"))
	{
		addRange(ESX_TEXT("Root"), 0x05000000, 0x1ABFFFFF, 0xFFFFFFFF);
		addRange(ESX_TEXT("Root"), 0x1FD00000, 0x602FFFFF, 0xFFFFFFFF);

		reset();

	}

	PIExternalBus::~PIExternalBus()
	{
	}

	void PIExternalBus::load(const StringView& busName, U32 address, U32& output, U8 lowerBits, U8 accessSize)
	{
		if (address >= 0x05000000 && address <= 0x05FFFFFF) {
			//TODO: N64DD Registers
		}
		else if (address >= 0x06000000 && address <= 0x07FFFFFF) {
			//TODO: N64DD IPL ROM
		}
		else if (address >= 0x08000000 && address <= 0x0FFFFFFF) {
			//TODO: Cartridge SRAM/FlashRAM
		}
		else if (address >= 0x10000000 && address <= 0x1FBFFFFF) {
			if (address - 0x10000000 < mFileSize) {
				fseek(mCartridge, address - 0x10000000, SEEK_SET);
				fread_s(&output, sizeof(U32), sizeof(U32), 1, mCartridge);
				output = _byteswap_ulong(output);
			}
		}
		else if (address >= 0x1FD00000 && address <= 0x1FFFFFFF) {
			//TODO: Unused
		}
		else if (address >= 0x20000000 && address <= 0x7FFFFFFF) {
			//TODO: Unused
		}
	}

	void PIExternalBus::reset()
	{
	}

	void PIExternalBus::loadGame(StringView path)
	{
		fopen_s(&mCartridge, path.data(), "rb");
		fseek(mCartridge, 0, SEEK_END);
		mFileSize = ftell(mCartridge);
		fseek(mCartridge, 0, SEEK_SET);
	}
	String PIExternalBus::getGameCode()
	{
		String gameCode = "TEST";
		fseek(mCartridge, 0x3B, SEEK_SET);
		fread_s(gameCode.data(), 4, sizeof(U8), 4, mCartridge);
		return String(gameCode);
	}
}
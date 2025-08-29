#include "RAM.h"

#include "VR4300.h"

namespace esx {



	RAM::RAM(const StringView& name, U32 startAddress, U32 addressingSize, U64 size, BIT checkLock)
		: BusDevice(name), mCheckLock(checkLock)
	{
		mMemory.resize(size);
		reset();

		addRange(ESX_TEXT("Root"), startAddress, addressingSize, 0xFFFFFFFF);
	}

	RAM::~RAM()
	{
	}

	void RAM::init()
	{
	}

	void RAM::store(const StringView& busName, U32 address, U8 value)
	{
		checkLocked(address);
		if (isHiZ(address)) return;
		mMemory[address & (mMemory.size() - 1)] = value;
	}

	void RAM::load(const StringView& busName, U32 address, U8& output)
	{
		checkLocked(address);
		output = mMemory[address & (mMemory.size() - 1)];
		if (isHiZ(address)) output = 0xFF;
	}

	void RAM::store(const StringView& busName, U32 address, U16 value)
	{
		checkLocked(address);
		if (isHiZ(address)) return;
		*reinterpret_cast<U16*>(&mMemory[address & (mMemory.size() - 1)]) = value;
	}

	void RAM::load(const StringView& busName, U32 address, U16& output)
	{
		checkLocked(address);
		output = *reinterpret_cast<U16*>(&mMemory[address & (mMemory.size() - 1)]);
		if (isHiZ(address)) output = 0xFFFF;
	}

	void RAM::store(const StringView& busName, U32 address, U32 value)
	{
		checkLocked(address);
		if (isHiZ(address)) return;
		*reinterpret_cast<U32*>(&mMemory[address & (mMemory.size() - 1)]) = value;
	}

	void RAM::load(const StringView& busName, U32 address, U32& output)
	{
		checkLocked(address);
		output = *reinterpret_cast<U32*>(&mMemory[address & (mMemory.size() - 1)]);
		if (isHiZ(address)) output = 0xFFFFFFFF;
	}

	void RAM::reset() {
		std::fill(mMemory.begin(), mMemory.end(), 0x00);
	}

	U8* RAM::getFastPointer(U32 address)
	{
		return &mMemory[address];
	}

	void RAM::checkLocked(U32 address)
	{
		if (mCheckLock) {
			getBus("Root")->getDevice<VR4300>("VR4300")->raiseException(ExceptionType::AddressErrorStore);
		}
	}

	BIT RAM::isHiZ(U32 address)
	{
		return ESX_FALSE;
	}

}
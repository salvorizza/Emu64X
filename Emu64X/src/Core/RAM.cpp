#include "RAM.h"

#include "VR4300/VR4300.h"

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
		mMemory[address & (mMemory.size() - 1)] = value;
	}

	void RAM::load(const StringView& busName, U32 address, U8& output)
	{
		output = mMemory[address & (mMemory.size() - 1)];
	}

	void RAM::store(const StringView& busName, U32 address, U16 value)
	{
		*reinterpret_cast<U16*>(&mMemory[address & (mMemory.size() - 1)]) = value;
	}

	void RAM::load(const StringView& busName, U32 address, U16& output)
	{
		output = *reinterpret_cast<U16*>(&mMemory[address & (mMemory.size() - 1)]);
	}

	void RAM::store(const StringView& busName, U32 address, U32 value)
	{
		*reinterpret_cast<U32*>(&mMemory[address & (mMemory.size() - 1)]) = value;
	}

	void RAM::load(const StringView& busName, U32 address, U32& output)
	{
		output = *reinterpret_cast<U32*>(&mMemory[address & (mMemory.size() - 1)]);
	}

	void RAM::reset() {
		std::fill(mMemory.begin(), mMemory.end(), 0x00);
	}

	U8* RAM::getFastPointer(U32 address)
	{
		return &mMemory[address];
	}

}
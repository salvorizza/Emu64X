#include "MIPSInterface.h"

#include "Core/MIPS/VR4300/VR4300.h"

namespace esx {

	MIPSInterface::MIPSInterface()
		: BusDevice(ESX_TEXT("MIPSInterface"))
	{
		addRange(ESX_TEXT("Root"), 0x04300000, 0x000FFFFF, 0xFFFFFFFF);

		reset();
	}

	MIPSInterface::~MIPSInterface()
	{
	}

	void MIPSInterface::clock(U64 clocks)
	{
		for (const auto& [targetClocks, interruptType] : mDelayedInterrupts) {
			if (clocks >= targetClocks) {
				mInterruptStatus |= (1 << static_cast<U8>(interruptType));
			}
		}
		std::erase_if(mDelayedInterrupts, [&](const Pair<U64, InterruptType>& pair) { return clocks >= pair.first; });
	}

	void MIPSInterface::store(const StringView& busName, U32 address, U32 value)
	{
		switch (address) {
			default: {
				case 0x0430000C: {
					setInterruptMask(value);
					break;
				}
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	void MIPSInterface::load(const StringView& busName, U32 address, U32& output)
	{
		switch (address) {
			case 0x04300008: {
				output = getInterruptStatus();
				break;
			}
			case 0x0430000C: {
				output = getInterruptMask();
				break;
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Load from address 0x{:08x} not implemented yet", mName, address);
				break;
			}
		}
	}

	void MIPSInterface::reset()
	{
	}

	void MIPSInterface::setInterrupt(InterruptType type, BIT prevValue, BIT newValue, U64 delay)
	{
		if (prevValue == ESX_FALSE && newValue == ESX_TRUE) {
			U64 clocks = getBus("Root")->getDevice<VR4300>("VR4300")->getClocks();
			mDelayedInterrupts.emplace_back(clocks + delay, type);
		}
	}

	void MIPSInterface::clearInterrupt(InterruptType type)
	{
		std::erase_if(mDelayedInterrupts, [&](const Pair<U64, InterruptType>& pair) { return pair.second == type; });
		mInterruptStatus &= ~(1 << static_cast<U8>(type));
	}

	void MIPSInterface::setInterruptMask(U32 value)
	{
		for (I32 i = 0; i < 5; i++) {
			if (value & (1 << (i * 2))) mInterruptMask &= ~(1 << i);
			if (value & (1 << (i * 2 + 1))) mInterruptMask |= 1 << i;
		}
	}

	U32 MIPSInterface::getInterruptMask()
	{
		return mInterruptMask;
	}

	U32 MIPSInterface::getInterruptStatus()
	{
		return mInterruptMask;
	}

}
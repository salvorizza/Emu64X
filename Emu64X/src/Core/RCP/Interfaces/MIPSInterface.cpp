#include "MIPSInterface.h"

#include "../RCP.h"
#include "Core/MIPS/VR4300/VR4300.h"

namespace esx {

	MIPSInterface::MIPSInterface(RCP* rcp)
		: mRCP(rcp)
	{
	}

	MIPSInterface::~MIPSInterface()
	{
	}

	void MIPSInterface::init()
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

	void MIPSInterface::store(U32 address, U32 value)
	{
		switch (address) {
			case 0x04300000: {
				MI_MODE_WRITE_Register writeReg;
				writeReg.write(value);

				if(writeReg.get(layouts::MI_MODE_WRITE_Register::Field::ClearUpper).as<BIT>() == ESX_TRUE) {
					MI_MODE.set(layouts::MI_MODE_Register::Field::Upper, ESX_FALSE);
				}

				if (writeReg.get(layouts::MI_MODE_WRITE_Register::Field::SetUpper).as<BIT>() == ESX_TRUE) {
					ESX_CORE_LOG_WARNING("{} - Upper mode not implemented yet", mName);

					MI_MODE.set(layouts::MI_MODE_Register::Field::Upper, ESX_TRUE);
				}

				if (writeReg.get(layouts::MI_MODE_WRITE_Register::Field::ClearEBus).as<BIT>() == ESX_TRUE) {
					MI_MODE.set(layouts::MI_MODE_Register::Field::Ebus, ESX_FALSE);
				}

				if (writeReg.get(layouts::MI_MODE_WRITE_Register::Field::SetEBus).as<BIT>() == ESX_TRUE) {
					ESX_CORE_LOG_WARNING("{} - EBus mode not implemented yet", mName);

					MI_MODE.set(layouts::MI_MODE_Register::Field::Ebus, ESX_TRUE);
				}

				if (writeReg.get(layouts::MI_MODE_WRITE_Register::Field::ClearRepeat).as<BIT>() == ESX_TRUE) {
					MI_MODE.set(layouts::MI_MODE_Register::Field::Repeat, ESX_FALSE);
				}

				if (writeReg.get(layouts::MI_MODE_WRITE_Register::Field::SetRepeat).as<BIT>() == ESX_TRUE) {
					ESX_CORE_LOG_WARNING("{} - Repeat mode not implemented yet", mName);

					MI_MODE.set(layouts::MI_MODE_Register::Field::Repeat, ESX_TRUE);
				}

				MI_MODE.set(layouts::MI_MODE_Register::Field::RepeatCount, writeReg.get(layouts::MI_MODE_WRITE_Register::Field::RepeatCount).as<U8>());

				if (writeReg.get(layouts::MI_MODE_WRITE_Register::Field::ClearDP).as<BIT>() == ESX_TRUE) {
					clearInterrupt(InterruptType::DP);
				}

				break;
			}

			case 0x0430000C: {
				setInterruptMask(value);
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	U32 MIPSInterface::load(U32 address)
	{
		switch (address) {
			case 0x04300000: {
				return MI_MODE.read();
			}
			case 0x04300004: {
				return MI_VERSION.read();
			}
			case 0x04300008: {
				return getInterruptStatus();
				break;
			}
			case 0x0430000C: {
				return getInterruptMask();
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
		MI_VERSION.write(0x02020102);
	}

	void MIPSInterface::setInterrupt(InterruptType type, BIT prevValue, BIT newValue, U64 delay)
	{
		if (prevValue == ESX_FALSE && newValue == ESX_TRUE) {
			U64 clocks = mRCP->getBus("Root")->getDevice<VR4300>("VR4300")->getClocks();
			mDelayedInterrupts.emplace_back(clocks + delay, type);
		}
	}

	void MIPSInterface::clearInterrupt(InterruptType type)
	{
		std::erase_if(mDelayedInterrupts, [&](const Pair<U64, InterruptType>& pair) { return pair.second == type; });
		mInterruptStatus &= ~(1 << static_cast<U8>(type));
	}

	BIT MIPSInterface::interruptPending()
	{
		return (mInterruptStatus & mInterruptMask) != 0 ? ESX_TRUE : ESX_FALSE;
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
		return mInterruptStatus;
	}

}
#include "RSP.h"

#include "../RCP.h"

namespace esx {



	RSP::RSP(RCP* rcp)
		: mRCP(rcp)
	{
	}

	RSP::~RSP()
	{
	}

	void RSP::clock(U64 clocks)
	{
		U64 newRCPClocks = RCP::CPUClocksToRCPClocks(clocks);
		while (mRCPClocks < newRCPClocks) {
			mCore->clock();
			mRCPClocks++;
		}

		mSU->clock(clocks);
		mVU->clock(clocks);
	}

	void RSP::init()
	{
		mCore = MakeShared<R4000>(mRCP);
		mSU = mCore->registerCoprocessor<ScalarUnit>(0, mCore.get(), mRCP);
		mVU = mCore->registerCoprocessor<VectorUnit>(2, mCore.get());

		mCore->setHalt(ESX_TRUE);
	}

	void RSP::store(U32 address, U32 value)
	{
		switch (address) {
			case 0x04040000: {
				mSU->setRegister(RegisterIndex(0), value);
				break;
			}

			case 0x04040004: {
				mSU->setRegister(RegisterIndex(1), value);
				break;
			}

			case 0x04040008: {
				mSU->setRegister(RegisterIndex(2), value);
				break;
			}

			case 0x0404000C: {
				mSU->setRegister(RegisterIndex(3), value);
				break;
			}

			case 0x04040010: {
				mSU->setRegister(RegisterIndex(4), value);
				break;
			}

			case 0x0404001C: {
				mSU->setRegister(RegisterIndex(7), value);
				break;
			}

			case 0x04080000: {
				value &= 0xFFC;
				
				mCore->mPC = value;
				mCore->mCurrentPC = mCore->mPC - 4;
				mCore->mNextPC = mCore->mPC + 4;
				break;
			}
		}
	}

	U32 RSP::load(U32 address)
	{
		switch (address) {
			case 0x04040000: {
				return mSU->getRegister(RegisterIndex(0));
			}

			case 0x04040004: {
				return mSU->getRegister(RegisterIndex(1));
			}

			case 0x04040008: {
				return mSU->getRegister(RegisterIndex(2));
			}

			case 0x0404000C: {
				return mSU->getRegister(RegisterIndex(3));
			}

			case 0x04040010: {
				return mSU->getRegister(RegisterIndex(4));
			}

			case 0x04040014: {
				return mSU->getRegister(RegisterIndex(5));
			}

			case 0x04040018: {
				return mSU->getRegister(RegisterIndex(6));
			}

			case 0x0404001C: {
				return mSU->getRegister(RegisterIndex(7));
			}

			case 0x04080000: {
				return mCore->mPC & 0xFFF;
			}
		}
	}

	void RSP::reset() {
		mCore->reset();
		mRCPClocks = 0;
	}

}
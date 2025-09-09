#include "RSP.h"

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
		mCore->clock();
		mSU->clock(clocks);
		mVU->clock(clocks);
	}

	void RSP::init()
	{
		mCore = MakeShared<R4000>();
		mSU = mCore->registerCoprocessor<ScalarUnit>(0, mCore.get());
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
				//mCore->setRegister(RegisterIndex(7), value);
				break;
			}
		}
	}

	U32 RSP::load(U32 address)
	{
		switch (address) {
			case 0x04040000: {
				return mSU->getRegister(RegisterIndex(0));
				break;
			}

			case 0x04040004: {
				return mSU->getRegister(RegisterIndex(1));
				break;
			}

			case 0x04040008: {
				return mSU->getRegister(RegisterIndex(2));
				break;
			}

			case 0x0404000C: {
				return mSU->getRegister(RegisterIndex(3));
				break;
			}

			case 0x04040010: {
				return mSU->getRegister(RegisterIndex(4));
				break;
			}

			case 0x04040014: {
				return mSU->getRegister(RegisterIndex(5));
				break;
			}

			case 0x04040018: {
				return mSU->getRegister(RegisterIndex(6));
				break;
			}

			case 0x0404001C: {
				return mSU->getRegister(RegisterIndex(7));
				break;
			}

			case 0x04080000: {
				//mCore->setRegister(RegisterIndex(7), value);
				break;
			}
		}
	}

	void RSP::reset() {
		mCore->reset();
	}

}
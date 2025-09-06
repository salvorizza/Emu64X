#include "RSP.h"

#include "Core/MIPS/R4000/R4000.h"

namespace esx {



	RSP::RSP()
		: BusDevice(ESX_TEXT("RSP"))
	{
		addRange(ESX_TEXT("Root"), 0x04040000, 0x000BC000, 0x0007FFFF);
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

	void RSP::store(const StringView& busName, U32 address, U32 value)
	{
		switch (address) {
			case 0x00040000: {
				mSU->setRegister(RegisterIndex(0), value);
				break;
			}

			case 0x00040004: {
				mSU->setRegister(RegisterIndex(1), value);
				break;
			}

			case 0x00040008: {
				mSU->setRegister(RegisterIndex(2), value);
				break;
			}

			case 0x0004000C: {
				mSU->setRegister(RegisterIndex(3), value);
				break;
			}

			case 0x00040010: {
				mSU->setRegister(RegisterIndex(4), value);
				break;
			}

			case 0x0004001C: {
				mSU->setRegister(RegisterIndex(7), value);
				break;
			}

			case 0x00080000: {
				//mCore->setRegister(RegisterIndex(7), value);
				break;
			}
		}
	}

	void RSP::load(const StringView& busName, U32 address, U32& output)
	{
		switch (address) {
			case 0x00040000: {
				output = mSU->getRegister(RegisterIndex(0));
				break;
			}

			case 0x00040004: {
				output = mSU->getRegister(RegisterIndex(1));
				break;
			}

			case 0x00040008: {
				output = mSU->getRegister(RegisterIndex(2));
				break;
			}

			case 0x0004000C: {
				output = mSU->getRegister(RegisterIndex(3));
				break;
			}

			case 0x00040010: {
				output = mSU->getRegister(RegisterIndex(4));
				break;
			}

			case 0x00040014: {
				output = mSU->getRegister(RegisterIndex(5));
				break;
			}

			case 0x00040018: {
				output = mSU->getRegister(RegisterIndex(6));
				break;
			}

			case 0x0004001C: {
				output = mSU->getRegister(RegisterIndex(7));
				break;
			}

			case 0x00080000: {
				//mCore->setRegister(RegisterIndex(7), value);
				break;
			}
		}
	}

	void RSP::reset() {
		mCore->reset();
	}

}
#include "Coprocessor.h"

#include "VR4300.h"

namespace esx {
	Coprocessor::Coprocessor(VR4300* cpu, U8 number)
		: mCPU(cpu), mNumber(number)
	{
	}

	void Coprocessor::clock(U64 clocks) {
		if(mMemoryLoad.first != -1) setRegister(mMemoryLoad.first, mMemoryLoad.second);
		mMemoryLoad = mPendingLoad;
		resetPendingLoad();
	}

	void Coprocessor::NA() {
		mCPU->raiseException(ExceptionType::ReservedInstruction);
	}

	void Coprocessor::MF() {
		if (!mCPU->mCP0.isCoprocessorUsable(mNumber)) {
			mCPU->raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		U64 r = 0;
		if (mNumber != 0 && mCPU->mCP0.is64BitMode()) {
			U8 rd = mCPU->mCurrentInstruction.RegisterDestination().Value;
			r = getRegister(RegisterIndex((rd >> 1) << 1));

			if ((rd & 0x1) == 0) {
				r = (r >> 0) & 0xFFFFFFFF;
			} else {
				r = (r >> 32) & 0xFFFFFFFF;
			}
		} else {
			r = getRegister(mCPU->mCurrentInstruction.RegisterDestination());
		}
		
		if (mCPU->mCP0.is64BitMode()) {
			r = static_cast<I64>(static_cast<I32>(static_cast<U32>(r)));
		}

		mCPU->addPendingLoad(mCPU->mCurrentInstruction.RegisterTarget(), r);
	}

	void Coprocessor::DMF() {
		if (!mCPU->mCP0.isCoprocessorUsable(mNumber)) {
			mCPU->raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		if (mCPU->mCP0.isReserved64BitInstruction()) {
			mCPU->raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 r = getRegister(mCPU->mCurrentInstruction.RegisterDestination());
		mCPU->addPendingLoad(mCPU->mCurrentInstruction.RegisterTarget(), r);
	}

	void Coprocessor::CF() {
		mCPU->raiseException(ExceptionType::CoprocessorUnusable);
	}

	void Coprocessor::MT() {
		if (!mCPU->mCP0.isCoprocessorUsable(mNumber)) {
			mCPU->raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		U64 data = mCPU->getRegister(mCPU->mCurrentInstruction.RegisterTarget());

		if (mNumber != 0 && mCPU->mCP0.is64BitMode()) {
			U8 rd = mCPU->mCurrentInstruction.RegisterDestination().Value;
			U64 r = getRegister(RegisterIndex((rd >> 1) << 1));
			data &= 0xFFFFFFFF;

			if ((rd & 0x1) == 0) {
				r = r & 0xFFFFFFFF00000000 | data;
			}
			else {
				r = (data << 32) | (r & 0xFFFFFFFF);
			}

			addPendingLoad(RegisterIndex((rd >> 1) << 1), r);
		}
		else {
			U64 r = data;
			addPendingLoad(mCPU->mCurrentInstruction.RegisterDestination(), r);
		}
	}

	void Coprocessor::DMT() {
		if (!mCPU->mCP0.isCoprocessorUsable(mNumber)) {
			mCPU->raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		if (mCPU->mCP0.isReserved64BitInstruction()) {
			mCPU->raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 r = mCPU->getRegister(mCPU->mCurrentInstruction.RegisterTarget());
		addPendingLoad(mCPU->mCurrentInstruction.RegisterDestination(), r);
	}

	void Coprocessor::CT() {
		mCPU->raiseException(ExceptionType::CoprocessorUnusable);
	}

	void Coprocessor::BCF() {
		mCPU->raiseException(ExceptionType::CoprocessorUnusable);
	}

	void Coprocessor::BCT() {
		mCPU->raiseException(ExceptionType::CoprocessorUnusable);
	}

	void Coprocessor::BCFL() {
		mCPU->raiseException(ExceptionType::CoprocessorUnusable);
	}

	void Coprocessor::BCTL() {
		mCPU->raiseException(ExceptionType::CoprocessorUnusable);
	}

	void Coprocessor::CO() {

	}

	void Coprocessor::addPendingLoad(RegisterIndex index, U64 value)
	{
		mPendingLoad.first = index;
		mPendingLoad.second = value;

		if (mMemoryLoad.first == index) {
			mMemoryLoad = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
		}
	}

	void Coprocessor::resetPendingLoad()
	{
		mPendingLoad.first = RegisterIndex(-1);
		mPendingLoad.second = 0;
	}

}
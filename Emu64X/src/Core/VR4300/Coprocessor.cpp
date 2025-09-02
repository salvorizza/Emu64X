#include "Coprocessor.h"

#include "VR4300.h"

namespace esx {
	Coprocessor::Coprocessor(U8 number)
		: mNumber(number)
	{
	}

	void Coprocessor::clock(U64 clocks) {
		if(mMemoryLoad.first != -1) setRegister(mMemoryLoad.first, mMemoryLoad.second);
		mMemoryLoad = mPendingLoad;
		resetPendingLoad();
	}

	void Coprocessor::NA(VR4300* cpu) {
		cpu->raiseException(ExceptionType::ReservedInstruction);
	}

	void Coprocessor::MF(VR4300* cpu) {
		if (!cpu->mCP0.isCoprocessorUsable(mNumber)) {
			cpu->raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		U64 r = 0;
		if (mNumber != 0 && cpu->mCP0.is64BitMode()) {
			U8 rd = cpu->mCurrentInstruction.RegisterDestination().Value;
			r = getRegister(RegisterIndex((rd >> 1) << 1));

			if ((rd & 0x1) == 0) {
				r = (r >> 0) & 0xFFFFFFFF;
			} else {
				r = (r >> 32) & 0xFFFFFFFF;
			}
		} else {
			r = getRegister(cpu->mCurrentInstruction.RegisterDestination());
		}
		
		if (cpu->mCP0.is64BitMode()) {
			r = static_cast<I64>(static_cast<I32>(static_cast<U32>(r)));
		}

		cpu->addPendingLoad(cpu->mCurrentInstruction.RegisterTarget(), r);
	}

	void Coprocessor::DMF(VR4300* cpu) {
		if (!cpu->mCP0.isCoprocessorUsable(mNumber)) {
			cpu->raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		if (cpu->mCP0.isReserved64BitInstruction()) {
			cpu->raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 r = getRegister(cpu->mCurrentInstruction.RegisterDestination());
		cpu->addPendingLoad(cpu->mCurrentInstruction.RegisterTarget(), r);
	}

	void Coprocessor::CF(VR4300* cpu) {
		cpu->raiseException(ExceptionType::CoprocessorUnusable);
	}

	void Coprocessor::MT(VR4300* cpu) {
		if (!cpu->mCP0.isCoprocessorUsable(mNumber)) {
			cpu->raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		U64 data = cpu->getRegister(cpu->mCurrentInstruction.RegisterTarget());

		if (mNumber != 0 && cpu->mCP0.is64BitMode()) {
			U8 rd = cpu->mCurrentInstruction.RegisterDestination().Value;
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
			addPendingLoad(cpu->mCurrentInstruction.RegisterDestination(), r);
		}
	}

	void Coprocessor::DMT(VR4300* cpu) {
		if (!cpu->mCP0.isCoprocessorUsable(mNumber)) {
			cpu->raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		if (cpu->mCP0.isReserved64BitInstruction()) {
			cpu->raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 r = cpu->getRegister(cpu->mCurrentInstruction.RegisterTarget());
		addPendingLoad(cpu->mCurrentInstruction.RegisterDestination(), r);
	}

	void Coprocessor::CT(VR4300* cpu) {
		cpu->raiseException(ExceptionType::CoprocessorUnusable);
	}

	void Coprocessor::BCF(VR4300* cpu) {
		cpu->raiseException(ExceptionType::CoprocessorUnusable);
	}

	void Coprocessor::BCT(VR4300* cpu) {
		cpu->raiseException(ExceptionType::CoprocessorUnusable);
	}

	void Coprocessor::BCFL(VR4300* cpu) {
		cpu->raiseException(ExceptionType::CoprocessorUnusable);
	}

	void Coprocessor::BCTL(VR4300* cpu) {
		cpu->raiseException(ExceptionType::CoprocessorUnusable);
	}

	void Coprocessor::CO(VR4300* cpu) {

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
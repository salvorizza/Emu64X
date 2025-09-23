#pragma once

#include "Base/Base.h"

namespace esx {

	class ICoprocessor {
	public:
		virtual ~ICoprocessor() = default;

		virtual void clock(U64 clocks) = 0;
		virtual void NA() = 0;
		virtual void MF() = 0;
		virtual void DMF() = 0;
		virtual void CF() = 0;
		virtual void MT() = 0;
		virtual void DMT() = 0;
		virtual void CT() = 0;
		virtual void BCF() = 0;
		virtual void BCT() = 0;
		virtual void BCFL() = 0;
		virtual void BCTL() = 0;
		virtual void CO() = 0;

		virtual U64 getRegister(RegisterIndex reg) = 0;
		virtual void setRegister(RegisterIndex reg, U64 value) = 0;
		virtual void unusable() = 0;
		virtual void reserved() = 0;
		virtual void signalBreak() = 0;
	protected:
		virtual void addPendingLoad(RegisterIndex index, U64 value) = 0;
		virtual void resetPendingLoad() = 0;
	};

	template<typename Processor>
	class Coprocessor : public ICoprocessor {
	public:
		Coprocessor(Processor* cpu, U8 number)
			: mCPU(cpu), mNumber(number)
		{
		}

		virtual ~Coprocessor() = default;

		void clock(U64 clocks) override {
		}

		void NA() override {
			reserved();
		}

		void MF() override {
			if (!mCPU->isCoprocessorUsable(mNumber)) {
				unusable();
				return;
			}

			U64 r = 0;
			if (mNumber != 0 && mCPU->is64BitMode()) {
				U8 rd = mCPU->mCurrentInstruction.RegisterDestination().Value;
				r = getRegister(RegisterIndex((rd >> 1) << 1));

				if ((rd & 0x1) == 0) {
					r = (r >> 0) & 0xFFFFFFFF;
				}
				else {
					r = (r >> 32) & 0xFFFFFFFF;
				}
			}
			else {
				r = getRegister(mCPU->mCurrentInstruction.RegisterDestination());
			}

			if (mCPU->is64BitMode()) {
				r = static_cast<I64>(static_cast<I32>(static_cast<U32>(r)));
			}

			mCPU->setRegister(mCPU->mCurrentInstruction.RegisterTarget(), r);
		}

		void DMF() override {
			if (!mCPU->isCoprocessorUsable(mNumber)) {
				unusable();
				return;
			}

			if (mCPU->isReserved64BitInstruction()) {
				reserved();
				return;
			}

			U64 r = getRegister(mCPU->mCurrentInstruction.RegisterDestination());
			mCPU->setRegister(mCPU->mCurrentInstruction.RegisterTarget(), r);
		}

		void CF() override {
			unusable();
		}

		void MT() override {
			if (!mCPU->isCoprocessorUsable(mNumber)) {
				unusable();
				return;
			}

			U64 data = mCPU->getRegister(mCPU->mCurrentInstruction.RegisterTarget());

			if (mNumber != 0 && mCPU->is64BitMode()) {
				U8 rd = mCPU->mCurrentInstruction.RegisterDestination().Value;
				U64 r = getRegister(RegisterIndex((rd >> 1) << 1));
				data &= 0xFFFFFFFF;

				if ((rd & 0x1) == 0) {
					r = r & 0xFFFFFFFF00000000 | data;
				}
				else {
					r = (data << 32) | (r & 0xFFFFFFFF);
				}

				setRegister(RegisterIndex((rd >> 1) << 1), r);
			}
			else {
				U64 r = data;
				setRegister(mCPU->mCurrentInstruction.RegisterDestination(), r);
			}
		}

		void DMT() override {
			if (!mCPU->isCoprocessorUsable(mNumber)) {
				unusable();
				return;
			}

			if (mCPU->isReserved64BitInstruction()) {
				reserved();
				return;
			}

			U64 r = mCPU->getRegister(mCPU->mCurrentInstruction.RegisterTarget());
			setRegister(mCPU->mCurrentInstruction.RegisterDestination(), r);
		}

		void CT() override {
			unusable();
		}

		void BCF() override {
			unusable();
		}

		void BCT() override {
			unusable();
		}

		void BCFL() override {
			unusable();
		}

		void BCTL() override {
			unusable();
		}

		void CO() override {

		}

		U64 getRegister(RegisterIndex reg) override { return 0; }
		void setRegister(RegisterIndex reg, U64 value) override {}

		void unusable() override {}
		void reserved() override {}
		void signalBreak() override {}
	protected:
		void addPendingLoad(RegisterIndex index, U64 value) override
		{
			mPendingLoad.first = index;
			mPendingLoad.second = value;

			if (mMemoryLoad.first == index) {
				mMemoryLoad = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
			}
		}

		void resetPendingLoad() override
		{
			mPendingLoad.first = RegisterIndex(-1);
			mPendingLoad.second = 0;
		}
	protected:
		Processor* mCPU;
	private:
		U8 mNumber;
		Pair<RegisterIndex, U64> mPendingLoad;
		Pair<RegisterIndex, U64> mMemoryLoad;
	};

	typedef void(ICoprocessor::*CoprocessorExecuteFunction)();

	static const Array<CoprocessorExecuteFunction, 32> copDecodeRS = {
		&ICoprocessor::MF,	&ICoprocessor::DMF,	&ICoprocessor::CF,	&ICoprocessor::NA,	&ICoprocessor::MT,	&ICoprocessor::DMT,	&ICoprocessor::CT,	&ICoprocessor::NA,
		&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,
		&ICoprocessor::CO,	&ICoprocessor::CO,	&ICoprocessor::CO,	&ICoprocessor::CO,	&ICoprocessor::CO,	&ICoprocessor::CO,	&ICoprocessor::CO,	&ICoprocessor::CO,
		&ICoprocessor::CO,	&ICoprocessor::CO,	&ICoprocessor::CO,	&ICoprocessor::CO,	&ICoprocessor::CO,	&ICoprocessor::CO,	&ICoprocessor::CO,	&ICoprocessor::CO
	};

	static const Array<CoprocessorExecuteFunction, 32> copDecodeBC = {
		&ICoprocessor::BCF,	&ICoprocessor::BCT,	&ICoprocessor::BCFL,	&ICoprocessor::BCTL,	&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,
		&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,		&ICoprocessor::NA,		&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,
		&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,		&ICoprocessor::NA,		&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,
		&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,		&ICoprocessor::NA,		&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA,	&ICoprocessor::NA
	};
}
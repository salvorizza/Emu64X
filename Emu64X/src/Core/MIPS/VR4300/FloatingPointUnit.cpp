#include "FloatingPointUnit.h"

#include "Core/MIPS/VR4300/VR4300.h"

namespace esx {

	FloatingPointUnit::FloatingPointUnit(VR4300* cpu)
		: Coprocessor(cpu, 1)
	{
		FCR0.set(layouts::FCR0Register::Field::Imp, 0x0B);
	}

	void FloatingPointUnit::clock(U64 clocks)
	{
	}

	void FloatingPointUnit::CF()
	{
		if (!mCPU->isCoprocessorUsable(1)) {
			unusable();
			return;
		}

		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();

		U64 temp = 0;
		switch (fs.Value) {
			case 0: 
				temp = FCR0.read();
				break;

			case 31:
				temp = FCR31.read();
				break;

			default:
				return;
		}

		if (mCPU->is64BitMode()) {
			temp = static_cast<I32>(static_cast<U32>(temp));
		}

		mCPU->setRegister(mCPU->mCurrentInstruction.RegisterTarget(), temp);
	}

	void FloatingPointUnit::CT()
	{
		if (!mCPU->isCoprocessorUsable(1)) {
			unusable();
			return;
		}

		U64 temp = mCPU->getRegister(mCPU->mCurrentInstruction.RegisterTarget());
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		
		if (fs.Value != 31) {
			return;
		}

		FCR31.write(temp);

		if (FCR31.get(layouts::FCR31Register::Field::Cause).as<U8>() & FCR31.get(layouts::FCR31Register::Field::Enables).as<U8>()) {
			mCPU->raiseException(ExceptionType::FloatingPoint);
		}
	}

	void FloatingPointUnit::CO()
	{
		if (!mCPU->isCoprocessorUsable(1)) {
			unusable();
			return;
		}

		U8 fmt = mCPU->mCurrentInstruction.RegisterSource().Value;
		U8 ft = mCPU->mCurrentInstruction.RegisterTarget().Value;
		U8 fs = mCPU->mCurrentInstruction.RegisterDestination().Value;
		U8 fd = mCPU->mCurrentInstruction.ShiftAmount();
		U8 function = mCPU->mCurrentInstruction.Function();

		switch (function)
		{
			case 0: {
				ADD();
				break;
			}
			case 1: {
				SUB();
				break;
			}
			case 2: {
				MUL();
				break;
			}
			case 3: {
				DIV();
				break;
			}
			case 4: {
				SQRT();
				break;
			}
			case 5: {
				ABS();
				break;
			}
			case 6: {
				MOV();
				break;
			}
			case 7: {
				NEG();
				break;
			}
			case 8: {
				ROUNDL();
				break;
			}
			case 9: {
				TRUNCL();
				break;
			}
			case 10: {
				CEILL();
				break;
			}
			case 11: {
				FLOORL();
				break;
			}
			case 12: {
				ROUNDW();
				break;
			}
			case 13: {
				TRUNCW();
				break;
			}
			case 14: {
				CEILW();
				break;
			}
			case 15: {
				FLOORW();
				break;
			}
			case 32: {
				CVTS();
				break;
			}
			case 33: {
				CVTD();
				break;
			}
			case 36: {
				CVTW();
				break;
			}
			case 37: {
				CVTL();
				break;
			}
			case 48:
			case 49:
			case 50:
			case 51:
			case 52:
			case 53:
			case 54:
			case 55:
			case 56:
			case 57:
			case 58:
			case 59:
			case 60:
			case 61:
			case 62:
			case 63: {
				C();
				break;
			}

			default: {
				reserved();
				break;
			}
		}
	}

	void FloatingPointUnit::ADD()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::SUB()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::MUL()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::DIV()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::SQRT()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::ABS()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::MOV()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::NEG()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::ROUNDL()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::TRUNCL()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::CEILL()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::FLOORL()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::ROUNDW()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::TRUNCW()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::CEILW()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::FLOORW()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::CVTS()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::CVTD()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::CVTW()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::CVTL()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::C()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void FloatingPointUnit::unusable()
	{
		mCPU->raiseException(ExceptionType::CoprocessorUnusable);
	}

	void FloatingPointUnit::reserved()
	{
		mCPU->raiseException(ExceptionType::ReservedInstruction);
	}

	U64 FloatingPointUnit::getRegister(RegisterIndex reg)
	{
		return U64();
	}

	void FloatingPointUnit::setRegister(RegisterIndex reg, U64 value)
	{
		int i = 0;
	}

}
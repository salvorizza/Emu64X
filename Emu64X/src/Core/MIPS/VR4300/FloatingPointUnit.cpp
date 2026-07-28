#include "FloatingPointUnit.h"

#include <cfenv>

#include "Core/MIPS/VR4300/VR4300.h"

namespace esx {

	FloatingPointUnit::FloatingPointUnit(VR4300* cpu)
		:	Coprocessor(cpu, 1),
			FGR({})
	{
		FCR0.set<layouts::FCR0Register::Imp>(0x0B);
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
				temp = mCPU->getRegister(mCPU->mCurrentInstruction.RegisterTarget());
				break;
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
	}

	void FloatingPointUnit::CO()
	{
		if (!mCPU->isCoprocessorUsable(1)) {
			unusable();
			return;
		}

		U8 function = mCPU->mCurrentInstruction.Function();

		U8 rm = FCR31.get<layouts::FCR31Register::RM>();
		int feRoundMode = FE_TONEAREST;
		switch (rm) {
			case 0: feRoundMode = FE_TONEAREST; break;
			case 1: feRoundMode = FE_TOWARDZERO; break;
			case 2: feRoundMode = FE_UPWARD; break;
			case 3: feRoundMode = FE_DOWNWARD; break;
		}

		int oldRoundMode = std::fegetround();
		std::fesetround(feRoundMode);

		std::feclearexcept(FE_ALL_EXCEPT);
		FCR31.set<layouts::FCR31Register::Cause>((U8)0);

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

		BIT fsEnabled = FCR31.get<layouts::FCR31Register::FS>();

		if (std::fetestexcept(FE_INEXACT)) signal(FPUException::I);
		if (std::fetestexcept(FE_UNDERFLOW)) signal(FPUException::U);
		if (std::fetestexcept(FE_OVERFLOW)) signal(FPUException::O);
		if (std::fetestexcept(FE_DIVBYZERO)) signal(FPUException::Z);
		if (std::fetestexcept(FE_INVALID) && !fsEnabled) signal(FPUException::V);

		std::fesetround(oldRoundMode);

		if (checkExceptions()) {
			mCPU->raiseException(ExceptionType::FloatingPoint);
		}
	}

	void FloatingPointUnit::ADD()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex ft = mCPU->mCurrentInstruction.RegisterTarget();
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, fmt, ValueFPR<F32>(fs, fmt) + ValueFPR<F32>(ft, fmt));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, fmt, ValueFPR<F64>(fs, fmt) + ValueFPR<F64>(ft, fmt));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::SUB()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex ft = mCPU->mCurrentInstruction.RegisterTarget();
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, fmt, ValueFPR<F32>(fs, fmt) - ValueFPR<F32>(ft, fmt));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, fmt, ValueFPR<F64>(fs, fmt) - ValueFPR<F64>(ft, fmt));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::MUL()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex ft = mCPU->mCurrentInstruction.RegisterTarget();
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, fmt, ValueFPR<F32>(fs, fmt) * ValueFPR<F32>(ft, fmt));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, fmt, ValueFPR<F64>(fs, fmt) * ValueFPR<F64>(ft, fmt));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::DIV()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex ft = mCPU->mCurrentInstruction.RegisterTarget();
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, fmt, ValueFPR<F32>(fs, fmt) / ValueFPR<F32>(ft, fmt));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, fmt, ValueFPR<F64>(fs, fmt) / ValueFPR<F64>(ft, fmt));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::SQRT()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, fmt, std::sqrtf(ValueFPR<F32>(fs, fmt)));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, fmt, std::sqrt(ValueFPR<F64>(fs, fmt)));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::ABS()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, fmt, std::fabsf(ValueFPR<F32>(fs, fmt)));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, fmt, std::fabs(ValueFPR<F64>(fs, fmt)));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::MOV()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, fmt, ValueFPR<F32>(fs, fmt));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, fmt, ValueFPR<F64>(fs, fmt));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::NEG()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, fmt, -ValueFPR<F32>(fs, fmt));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, fmt, -ValueFPR<F64>(fs, fmt));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::ROUNDL()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		int oldRm = std::fegetround();
		std::fesetround(FE_TONEAREST);

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, FormatSpec::L, ConvertFmt<U64>(std::nearbyint(ValueFPR<F32>(fs, fmt)), fmt, FormatSpec::L));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, FormatSpec::L, ConvertFmt<U64>(std::nearbyint(ValueFPR<F64>(fs, fmt)), fmt, FormatSpec::L));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}

		std::fesetround(oldRm);
	}

	void FloatingPointUnit::TRUNCL()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, FormatSpec::L, ConvertFmt<U64>(std::trunc(ValueFPR<F32>(fs, fmt)), fmt, FormatSpec::L));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, FormatSpec::L, ConvertFmt<U64>(std::trunc(ValueFPR<F64>(fs, fmt)), fmt, FormatSpec::L));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::CEILL()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, FormatSpec::L, ConvertFmt<U64>(std::ceil(ValueFPR<F32>(fs, fmt)), fmt, FormatSpec::L));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, FormatSpec::L, ConvertFmt<U64>(std::ceil(ValueFPR<F64>(fs, fmt)), fmt, FormatSpec::L));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::FLOORL()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, FormatSpec::L, ConvertFmt<U64>(std::floor(ValueFPR<F32>(fs, fmt)), fmt, FormatSpec::L));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, FormatSpec::L, ConvertFmt<U64>(std::floor(ValueFPR<F64>(fs, fmt)), fmt, FormatSpec::L));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::ROUNDW()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		int oldRm = std::fegetround();
		std::fesetround(FE_TONEAREST);

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, FormatSpec::W, ConvertFmt<U32>(std::nearbyint(ValueFPR<F32>(fs, fmt)), fmt, FormatSpec::W));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, FormatSpec::W, ConvertFmt<U32>(std::nearbyint(ValueFPR<F64>(fs, fmt)), fmt, FormatSpec::W));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}

		std::fesetround(oldRm);
	}

	void FloatingPointUnit::TRUNCW()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, FormatSpec::W, ConvertFmt<U32>(std::trunc(ValueFPR<F32>(fs, fmt)), fmt, FormatSpec::W));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, FormatSpec::W, ConvertFmt<U32>(std::trunc(ValueFPR<F64>(fs, fmt)), fmt, FormatSpec::W));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::CEILW()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, FormatSpec::W, ConvertFmt<U32>(std::ceil(ValueFPR<F32>(fs, fmt)), fmt, FormatSpec::W));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, FormatSpec::W, ConvertFmt<U32>(std::ceil(ValueFPR<F64>(fs, fmt)), fmt, FormatSpec::W));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::FLOORW()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, FormatSpec::W, ConvertFmt<U32>(std::floor(ValueFPR<F32>(fs, fmt)), fmt, FormatSpec::W));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, FormatSpec::W, ConvertFmt<U32>(std::floor(ValueFPR<F64>(fs, fmt)), fmt, FormatSpec::W));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::CVTS()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::D: {
				StoreFPR(fd, FormatSpec::S, ConvertFmt<F32>(ValueFPR<F64>(fs, fmt), fmt, FormatSpec::S));
				break;
			}

			case FormatSpec::W: {
				StoreFPR(fd, FormatSpec::S, ConvertFmt<F32>(ValueFPR<U32>(fs, fmt), fmt, FormatSpec::S));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::CVTD()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, FormatSpec::D, ConvertFmt<F64>(ValueFPR<F32>(fs, fmt), fmt, FormatSpec::D));
				break;
			}

			case FormatSpec::W: {
				StoreFPR(fd, FormatSpec::D, ConvertFmt<F64>(ValueFPR<U32>(fs, fmt), fmt, FormatSpec::D));
				break;
			}

			case FormatSpec::L: {
				StoreFPR(fd, FormatSpec::D, ConvertFmt<F64>(ValueFPR<U64>(fs, fmt), fmt, FormatSpec::D));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::CVTW()
	{

		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, FormatSpec::W, ConvertFmt<U32>(ValueFPR<F32>(fs, fmt), fmt, FormatSpec::W));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, FormatSpec::W, ConvertFmt<U32>(ValueFPR<F64>(fs, fmt), fmt, FormatSpec::W));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::CVTL()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		RegisterIndex fd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

		switch (fmt) {
			case FormatSpec::S: {
				StoreFPR(fd, FormatSpec::L, ConvertFmt<U64>(ValueFPR<F32>(fs, fmt), fmt, FormatSpec::L));
				break;
			}

			case FormatSpec::D: {
				StoreFPR(fd, FormatSpec::L, ConvertFmt<U64>(ValueFPR<F64>(fs, fmt), fmt, FormatSpec::L));
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}
	}

	void FloatingPointUnit::C()
	{
		FormatSpec fmt = static_cast<FormatSpec>(mCPU->mCurrentInstruction.RegisterSource().Value);
		RegisterIndex ft = mCPU->mCurrentInstruction.RegisterTarget();
		RegisterIndex fs = mCPU->mCurrentInstruction.RegisterDestination();
		U8 cond = mCPU->mCurrentInstruction.Cond();

		BIT less = ESX_FALSE;
		BIT equal = ESX_FALSE;
		BIT unordered = ESX_FALSE;

		switch (fmt) {
			case FormatSpec::S: {
				if (std::isnan(ValueFPR<F32>(fs, fmt)) || std::isnan(ValueFPR<F32>(ft, fmt))) {
					less = ESX_FALSE;
					equal = ESX_FALSE;
					unordered = ESX_TRUE;

					if ((cond >> 3) & 0x1) {
						signal(FPUException::V);
					}
				} else {
					less = ValueFPR<F32>(fs, fmt) < ValueFPR<F32>(ft, fmt);
					equal = ValueFPR<F32>(fs, fmt) == ValueFPR<F32>(ft, fmt);
					unordered = ESX_FALSE;
				}

				break;
			}

			case FormatSpec::D: {
				if (std::isnan(ValueFPR<F64>(fs, fmt)) || std::isnan(ValueFPR<F64>(ft, fmt))) {
					less = ESX_FALSE;
					equal = ESX_FALSE;
					unordered = ESX_TRUE;

					if ((cond >> 3) & 0x1) {
						signal(FPUException::V);
					}
				} else {
					less = ValueFPR<F64>(fs, fmt) < ValueFPR<F64>(ft, fmt);
					equal = ValueFPR<F64>(fs, fmt) == ValueFPR<F64>(ft, fmt);
					unordered = ESX_FALSE;
				}
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("{} Not implemented yet with fmt {}", __FUNCTION__, static_cast<U8>(fmt));
				signal(FPUException::E);
				break;
			}
		}

		BIT condition = (((cond >> 2) & 0x1) && less) || (((cond >> 1) & 0x1) && equal) || (((cond >> 0) & 0x1) && unordered);
		FCR31.set<layouts::FCR31Register::C>(condition);
	}

	void FloatingPointUnit::unusable()
	{
		mCPU->raiseException(ExceptionType::CoprocessorUnusable, mNumber);
	}

	void FloatingPointUnit::reserved()
	{
		signal(FPUException::E);
	}

	void FloatingPointUnit::signal(FPUException exception)
	{
		U8 cause = FCR31.get<layouts::FCR31Register::Cause>();
		cause |= 1 << static_cast<U8>(exception);
		FCR31.set<layouts::FCR31Register::Cause>(cause);
	}

	BIT FloatingPointUnit::checkExceptions()
	{
		U8 cause = FCR31.get<layouts::FCR31Register::Cause>();
		U8 enables = FCR31.get<layouts::FCR31Register::Enables>();
		U8 exceptions = cause & enables;
		U8 flagBits = cause & (~enables);
		U8 oldFlags = FCR31.get<layouts::FCR31Register::Flags>();
		FCR31.set<layouts::FCR31Register::Flags>(oldFlags | flagBits);

		return exceptions != 0;
	}

	U64 FloatingPointUnit::getRegister(RegisterIndex reg)
	{
		if (mCPU->mCP0->useAdditionalFPR() == ESX_TRUE) {
			return FGR[reg.Value];
		} else {
			U8 even = reg.Value & ~1;
			return (FGR[even + 1] << 32) | FGR[even];
		}
	}

	void FloatingPointUnit::setRegister(RegisterIndex reg, U64 value)
	{
		if (mCPU->mCP0->useAdditionalFPR() == ESX_TRUE) {
			FGR[reg.Value] = value;
		} else {
			U8 even = reg.Value & ~1;
			FGR[even + 0] = (value >> 0)  & 0xFFFFFFFF;
			FGR[even + 1] = (value >> 32) & 0xFFFFFFFF;
		}
	}

	BIT FloatingPointUnit::COC()
	{
		return FCR31.get<layouts::FCR31Register::C>();
	}

}
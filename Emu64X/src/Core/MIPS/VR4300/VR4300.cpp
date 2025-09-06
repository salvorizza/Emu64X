#include "VR4300.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <intrin.h>

#include "optick.h"

#pragma intrinsic(_mul128)
#pragma intrinsic(_umul128)

namespace esx {

	VR4300::VR4300()
		:	MIPSProcessor(ESX_TEXT("VR4300"))
	{
		mCP0 = registerCoprocessor<SystemControlCoprocessor>(0, this);
	}

	VR4300::~VR4300()
	{
	}

	void VR4300::init()
	{

		mRootBus = getBus(ESX_TEXT("Root"));
	}

	void VR4300::clock()
	{
		MIPSProcessor::clock();
		mCP0->clock(mCycles);
		mCP0->handleInterrupts();
	}


	U32 VR4300::fetch(U32 virtualAddress)
	{
		if (ADDRESS_UNALIGNED(virtualAddress, U32)) {
			raiseException(ExceptionType::AddressErrorLoad, virtualAddress);
			return 0;
		}

		BIT cached = ESX_FALSE;
		U32 physicalAddress = mCP0->AddressTranslation(virtualAddress, ESX_FALSE, cached);

		if (cached) {
			U32 index = (virtualAddress >> 2) & 0x7;
			U32 cacheLineNumber = (virtualAddress >> 5) & 0x1FF;
			U32 tag = physicalAddress >> 12;

			auto& cacheLine = mICache.CacheLines[cacheLineNumber];
			if (cacheLine.Tag == tag && cacheLine.Valid) {
				auto& instruction = cacheLine.Instructions[index];
				return instruction.Word;
			} else {
				return cacheMiss(virtualAddress, physicalAddress, cacheLineNumber, tag, index);
			}
		} else {
			return mRootBus->load<U32>(physicalAddress);
		}
	}


	U32 VR4300::cacheMiss(U32 virtualAddress, U32 physicalAddress, U32 cacheLineNumber, U32 tag, U32 startIndex)
	{
		auto& cacheLine = mICache.CacheLines[cacheLineNumber];

		U32 baseAddr = physicalAddress & ~0x1F;

		for (U32 index = 0; index < cacheLine.Instructions.size(); index++) {
			auto& instruction = cacheLine.Instructions[index];
			instruction.Word = mRootBus->load<U32>(baseAddr + index * sizeof(U32));
		}

		cacheLine.Tag = tag;
		cacheLine.Valid = ESX_TRUE;

		return cacheLine.Instructions[startIndex].Word;
	}

	static const Array<VR4300ExecuteFunction, 64> primaryOpCodeDecode = {
		&VR4300::NA,	&VR4300::NA,		&VR4300::J,		&VR4300::JAL,	&VR4300::BEQ,	&VR4300::BNE,	&VR4300::BLEZ,	&VR4300::BGTZ,
		&VR4300::ADDI,	&VR4300::ADDIU,		&VR4300::SLTI,	&VR4300::SLTIU, &VR4300::ANDI,	&VR4300::ORI,	&VR4300::XORI,	&VR4300::LUI,
		&VR4300::COP0,	&VR4300::COP1,		&VR4300::COP2,	&VR4300::NA,	&VR4300::BEQL,	&VR4300::BNEL,	&VR4300::BLEZL,	&VR4300::BGTZL,
		&VR4300::DADDI,	&VR4300::DADDIU,	&VR4300::LDL,	&VR4300::LDR,	&VR4300::NA,	&VR4300::NA,	&VR4300::NA,	&VR4300::NA,
		&VR4300::LB,	&VR4300::LH,		&VR4300::LWL,	&VR4300::LW,	&VR4300::LBU,	&VR4300::LHU,	&VR4300::LWR,	&VR4300::LWU,
		&VR4300::SB,	&VR4300::SH,		&VR4300::SWL,	&VR4300::SW,	&VR4300::SDL,	&VR4300::SDR,	&VR4300::SWR,	&VR4300::CACHE,
		&VR4300::LL,	&VR4300::LWC1,		&VR4300::LWC2,	&VR4300::NA,	&VR4300::LLD,	&VR4300::LDC1,	&VR4300::LDC2,	&VR4300::LD,
		&VR4300::SC,	&VR4300::SWC1,		&VR4300::SWC2,	&VR4300::NA,	&VR4300::SCD,	&VR4300::SDC1,	&VR4300::SDC2,	&VR4300::SD
	};

	static const Array<VR4300ExecuteFunction, 64> secondaryOpCodeDecode = {
		&VR4300::SLL,	&VR4300::NA,	&VR4300::SRL,	&VR4300::SRA,	&VR4300::SLLV,		&VR4300::NA,		&VR4300::SRLV,		&VR4300::SRAV,
		&VR4300::JR,	&VR4300::JALR,	&VR4300::NA,	&VR4300::NA,	&VR4300::SYSCALL,	&VR4300::BREAK,		&VR4300::NA,		&VR4300::SYNC,
		&VR4300::MFHI,	&VR4300::MTHI,	&VR4300::MFLO,	&VR4300::MTLO,	&VR4300::DSLLV,		&VR4300::NA,		&VR4300::DSRLV,		&VR4300::DSRAV,
		&VR4300::MULT,	&VR4300::MULTU, &VR4300::DIV,	&VR4300::DIVU,	&VR4300::DMULT,		&VR4300::DMULTU,	&VR4300::DDIV,		&VR4300::DDIVU,
		&VR4300::ADD,	&VR4300::ADDU,	&VR4300::SUB,	&VR4300::SUBU,	&VR4300::AND,		&VR4300::OR,		&VR4300::XOR,		&VR4300::NOR,
		&VR4300::NA,	&VR4300::NA,	&VR4300::SLT,	&VR4300::SLTU,	&VR4300::DADD,		&VR4300::DADDU,		&VR4300::DSUB,		&VR4300::DSUBU,
		&VR4300::TGE,	&VR4300::TGEU,	&VR4300::TLT,	&VR4300::TLTU,	&VR4300::TEQ,		&VR4300::NA,		&VR4300::TNE,		&VR4300::NA,
		&VR4300::DSLL,	&VR4300::NA,	&VR4300::DSRL,	&VR4300::DSRA,	&VR4300::DSLL32,	&VR4300::NA,		&VR4300::DSRL32,	&VR4300::DSRA32
	};

	static const Array<VR4300ExecuteFunction, 32> branchOpCodeDecode = {
		&VR4300::BLTZ,		&VR4300::BGEZ,		&VR4300::BLTZL,		&VR4300::BGEZL,		&VR4300::NA,	&VR4300::NA,	&VR4300::NA,	&VR4300::NA,
		&VR4300::TGEI,		&VR4300::TGEIU,		&VR4300::TLTI,		&VR4300::TLTIU,		&VR4300::TEQI,	&VR4300::NA,	&VR4300::TNEI,	&VR4300::NA,
		&VR4300::BLTZAL,	&VR4300::BGEZAL,	&VR4300::BLTZALL,	&VR4300::BGEZALL,	&VR4300::NA,	&VR4300::NA,	&VR4300::NA,	&VR4300::NA,
		&VR4300::NA,		&VR4300::NA,		&VR4300::NA,		&VR4300::NA,		&VR4300::NA,	&VR4300::NA,	&VR4300::NA,	&VR4300::NA
	};

	void VR4300::decode(VR4300Instruction& result, U32 instruction, U32 address, BIT suppressException)
	{
		result.Address = address;
		result.binaryInstruction = instruction;
		result.Execute = nullptr;

		U32 primaryOpCode = result.Opcode();
		U32 secondaryOpCode = result.Function();

		switch (primaryOpCode) {
			case 0x00: {
				if (secondaryOpCode < secondaryOpCodeDecode.size()) {
					result.Execute = secondaryOpCodeDecode[secondaryOpCode];
				}
				break;
			}

			case 0x01: {
				if (result.RegisterTarget().Value < branchOpCodeDecode.size()) {
					result.Execute = branchOpCodeDecode[result.RegisterTarget().Value];
				}
				break;
			}

			default: {
				if (primaryOpCode < primaryOpCodeDecode.size()) {
					result.Execute = primaryOpCodeDecode[primaryOpCode];
				}
			}
		}
	}

	void VR4300::execute(VR4300Instruction& instruction)
	{
		(this->*instruction.Execute)();
	}

	void VR4300::reset()
	{
		MIPSProcessor::reset();
		mCP0->setRegister(SystemControlRegisterType::PRId, 0x000000B0);
	}

	void VR4300::ADD()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = getRegister(mCurrentInstruction.RegisterTarget());

		I32 r = a + b;

		if (OVERFLOW_ADD32(a, b, r)) {
			raiseException(ExceptionType::ArithmeticOverflow);
			return;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::ADDU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U64 r = a + b;

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		} else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::DADD()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I64 b = getRegister(mCurrentInstruction.RegisterTarget());

		I64 r = a + b;

		if (OVERFLOW_ADD64(a, b, r)) {
			raiseException(ExceptionType::ArithmeticOverflow);
			return;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::DADDU()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = getRegister(mCurrentInstruction.RegisterTarget());

		U64 r = a + b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SUB()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = getRegister(mCurrentInstruction.RegisterTarget());

		I32 r = a - b;

		if (OVERFLOW_SUB32(a, b, r)) {
			raiseException(ExceptionType::ArithmeticOverflow);
			return;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SUBU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U64 r = a - b;

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		} else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::DSUB()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I64 b = getRegister(mCurrentInstruction.RegisterTarget());

		I64 r = a - b;

		if (OVERFLOW_SUB64(a, b, r)) {
			raiseException(ExceptionType::ArithmeticOverflow);
			return;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::DSUBU()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = getRegister(mCurrentInstruction.RegisterTarget());

		U64 r = a - b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::ADDI()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = mCurrentInstruction.ImmediateSE();

		I32 r = a + b;

		if (OVERFLOW_ADD32(a, b, r)) {
			raiseException(ExceptionType::ArithmeticOverflow);
			return;
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::ADDIU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U64 r = a + b;

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		} else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::DADDI()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I64 b = mCurrentInstruction.ImmediateSE();

		I64 r = a + b;

		if (OVERFLOW_ADD64(a, b, r)) {
			raiseException(ExceptionType::ArithmeticOverflow);
			return;
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::DADDIU()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 r = a + b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::MULT()
	{
		I64 a = static_cast<I64>(static_cast<I32>(getRegister(mCurrentInstruction.RegisterSource())));
		I64 b = static_cast<I64>(static_cast<I32>(getRegister(mCurrentInstruction.RegisterTarget())));

		U64 r = static_cast<U64>(a * b);

		mHI = (r >> 32) & 0xFFFFFFFF;
		mLO = r & 0xFFFFFFFF;

		if (mCP0->is64BitMode()) {
			mHI = static_cast<I32>(mHI);
			mLO = static_cast<I32>(mLO);
		}
	}

	void VR4300::MULTU()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource()) & 0xFFFFFFFF;
		U64 b = getRegister(mCurrentInstruction.RegisterTarget()) & 0xFFFFFFFF;

		U64 r = a * b;

		mHI = (r >> 32) & 0xFFFFFFFF;
		mLO = r & 0xFFFFFFFF;

		if (mCP0->is64BitMode()) {
			mHI = static_cast<I32>(mHI);
			mLO = static_cast<I32>(mLO);
		}
	}

	void VR4300::DMULT()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		I64 a = static_cast<I64>(getRegister(mCurrentInstruction.RegisterSource()));
		I64 b = static_cast<I64>(getRegister(mCurrentInstruction.RegisterTarget()));

		I64 lo, hi;
		lo = _mul128(a, b, &hi);

		mHI = hi;
		mLO = lo;
	}

	void VR4300::DMULTU()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = getRegister(mCurrentInstruction.RegisterTarget());

		mLO = _umul128(a, b, &mHI);
	}

	void VR4300::DIV()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = getRegister(mCurrentInstruction.RegisterTarget());

		if (b != 0) {
			if (a == 0x80000000 && b == -1) {
				mHI = 0;
				mLO = 0x80000000;
			} else {
				mHI = a % b;
				mLO = a / b;
			}
		} else {
			mHI = a;

			if (a >= 0) {
				mLO = 0x7FFFFFFF;
			} else {
				mLO = 0x80000001;
			}
		}

		if (mCP0->is64BitMode()) {
			mHI = static_cast<I32>(mHI);
			mLO = static_cast<I32>(mLO);
		}
	}

	void VR4300::DIVU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		if (b != 0) {
			mHI = a % b;
			mLO = a / b;
		} else {
			mHI = a;
			mLO = 0x7FFFFFFF;
		}

		if (mCP0->is64BitMode()) {
			mHI = static_cast<I32>(mHI);
			mLO = static_cast<I32>(mLO);
		}
	}

	void VR4300::DDIV()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I64 b = getRegister(mCurrentInstruction.RegisterTarget());

		if (b != 0) {
			if (a == 0x8000000000000000 && b == -1) {
				mHI = 0;
				mLO = 0x8000000000000000;
			}
			else {
				mHI = a % b;
				mLO = a / b;
			}
		}
		else {
			mHI = a;

			if (a >= 0) {
				mLO = 0x7FFFFFFF;
			} else {
				mLO = 0x80000001;
			}
			mLO = static_cast<I32>(mLO);
		}
	}

	void VR4300::DDIVU()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = getRegister(mCurrentInstruction.RegisterTarget());

		if (b != 0) {
			mHI = a % b;
			mLO = a / b;
		} else {
			mHI = a;
			mLO = 0x7FFFFFFF;
		}
	}

	void VR4300::MFLO()
	{
		setRegister(mCurrentInstruction.RegisterDestination(), mLO);
	}

	void VR4300::MTLO()
	{
		mLO = getRegister(mCurrentInstruction.RegisterSource());
	}

	void VR4300::MFHI()
	{
		setRegister(mCurrentInstruction.RegisterDestination(), mHI);
	}

	void VR4300::MTHI()
	{
		mHI = getRegister(mCurrentInstruction.RegisterSource());
	}

	void VR4300::LD()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 r = load<U64>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LDL()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 c = getRegister(mCurrentInstruction.RegisterTarget());
		if (mMemoryLoad.first == mCurrentInstruction.RegisterTarget()) {
			c = mMemoryLoad.second;
		}

		U64 m = a + b;

		U64 am = m & ~(0x7);
		U64 aw = load<U64>(am, exception);

		U64 u = m & 0x7;
		U64 r = (c & (0x00FFFFFFFFFFFFFF >> (u * 8))) | (aw << (56 - (u * 8)));

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LDR()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 c = getRegister(mCurrentInstruction.RegisterTarget());
		if (mMemoryLoad.first == mCurrentInstruction.RegisterTarget()) {
			c = mMemoryLoad.second;
		}

		U64 m = a + b;

		U64 am = m & ~(0x7);
		U64 aw = load<U64>(am, exception);

		U64 u = m & 0x7;
		U64 r = (c & (0xFFFFFFFFFFFFFF00 << ((0x7 - u) * 8))) | (aw >> (u * 8));

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LW()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 r = load<U32>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		} else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LWU()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 r = load<U32>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LH()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 r = load<U16>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		} else {
			r = static_cast<U32>(static_cast<I32>(static_cast<I16>(static_cast<U16>(r))));

			if (mCP0->is64BitMode()) {
				r = static_cast<I32>(static_cast<U32>(r));
			} else {
				r &= 0xFFFFFFFF;
			}
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LHU()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U32 r = load<U16>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LB()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 r = load<U8>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		} else {
			r = static_cast<U32>(static_cast<I32>(static_cast<I8>(static_cast<U8>(r))));

			if (mCP0->is64BitMode()) {
				r = static_cast<I32>(static_cast<U32>(r));
			} else {
				r &= 0xFFFFFFFF;
			}
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LBU()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U32 r = load<U8>(m, exception);

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LWL()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 c = getRegister(mCurrentInstruction.RegisterTarget());
		if (mMemoryLoad.first == mCurrentInstruction.RegisterTarget()) {
			c = mMemoryLoad.second;
		}

		U64 m = a + b;

		U64 am = m & ~(0x3);
		U64 aw = load<U32>(am, exception);

		U64 u = m & 0x3;
		U64 r = (c & (0x00FFFFFF >> (u * 8))) | (aw << (24 - (u * 8)));

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		} else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::SWL()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 c = getRegister(mCurrentInstruction.RegisterTarget());

		U64 m = a + b;

		U64 am = m & ~(0x3);
		U64 aw = load<U32>(am, exception);

		U64 u = (m & 0x3) * 8;
		U64 mr = (aw & (0xFFFFFF00 << u)) | (c >> (24 - u));

		store<U32>(am, mr);
	}


	void VR4300::LWR()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 c = getRegister(mCurrentInstruction.RegisterTarget());
		if (mMemoryLoad.first == mCurrentInstruction.RegisterTarget()) {
			c = mMemoryLoad.second;
		}

		U64 m = a + b;

		U64 am = m & ~(0x3);
		U64 aw = load<U32>(am, exception);

		U64 u = m & 0x3;
		U64 r = (c & (0xFFFFFF00 << ((0x3 - u) * 8))) | (aw >> (u * 8));

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		} else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::SWR()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 c = getRegister(mCurrentInstruction.RegisterTarget());

		U64 m = a + b;

		U64 am = m & ~(0x3);
		U64 aw = load<U32>(am, exception);

		U64 u = m & 0x3;
		U64 mr = (aw & (0x00FFFFFF >> ((0x3 - u) * 8))) | (c << (u * 8));

		store<U32>(am, mr);
	}

	void VR4300::SB()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 v = getRegister(mCurrentInstruction.RegisterTarget());

		U64 m = a + b;

		store<U8>(m, v);
	}

	void VR4300::SCD()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 v = getRegister(mCurrentInstruction.RegisterTarget());

		if (mLLBit == ESX_TRUE) {
			store<U64>(m, v);
		}

		setRegister(mCurrentInstruction.RegisterTarget(), mLLBit);
	}

	void VR4300::SH()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 v = getRegister(mCurrentInstruction.RegisterTarget());

		U64 m = a + b;

		store<U16>(m, v);
	}

	void VR4300::SC()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 v = getRegister(mCurrentInstruction.RegisterTarget());

		if (mLLBit == ESX_TRUE) {
			store<U32>(m, v);
		}

		setRegister(mCurrentInstruction.RegisterTarget(), mLLBit);
	}

	void VR4300::SD()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 v = getRegister(mCurrentInstruction.RegisterTarget());

		store<U64>(m, v);
	}

	void VR4300::SDL()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 c = getRegister(mCurrentInstruction.RegisterTarget());

		U64 m = a + b;

		U64 am = m & ~(0x7);
		U64 aw = load<U64>(am, exception);

		U64 u = (m & 0x7) * 8;
		U64 mr = (aw & (0xFFFFFFFFFFFFFF00 << u)) | (c >> (56 - u));

		store<U64>(am, mr);
	}

	void VR4300::SDR()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 c = getRegister(mCurrentInstruction.RegisterTarget());

		U64 m = a + b;

		U64 am = m & ~(0x7);
		U64 aw = load<U32>(am, exception);

		U64 u = m & 0x7;
		U64 mr = (aw & (0x00FFFFFFFFFFFFFF >> ((0x7 - u) * 8))) | (c << (u * 8));

		store<U64>(am, mr);
	}

	void VR4300::SW()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 v = getRegister(mCurrentInstruction.RegisterTarget());

		store<U32>(m, v);
	}

	void VR4300::LUI()
	{
		U64 r = mCurrentInstruction.Immediate() << 16;

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		} else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LL()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 r = load<U32>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		}
		else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
		mLLBit = ESX_TRUE;
		mCP0->setLLAddrToLastTranslation();
	}

	void VR4300::LLD()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 r = load<U64>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
		mLLBit = ESX_TRUE;
		mCP0->setLLAddrToLastTranslation();
	}

	void VR4300::SLT()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a < b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SLTU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a < b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SLTI()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = mCurrentInstruction.ImmediateSE();

		U32 r = a < b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::SLTIU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 r = a < b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::AND()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a & b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::ANDI()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.Immediate();

		U32 r = a & b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::OR()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a | b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::ORI()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.Immediate();

		U32 r = a | b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::XOR()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a ^ b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::XORI()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.Immediate();

		U32 r = a ^ b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::NOR()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = ~(a | b);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SLL()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U64 r = a << s;

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		} else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::DSLL()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U64 r = a << s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::DSLL32()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = 32 + mCurrentInstruction.ShiftAmount();

		U64 r = a << s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SRL()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U64 r = a >> s;

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		} else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::DSRL()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U64 r = a >> s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::DSRL32()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = 32 + mCurrentInstruction.ShiftAmount();

		U64 r = a >> s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SRA()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U64 r = a >> s;

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		} else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::DSRA()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		I64 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U64 r = a >> s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::DSRA32()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		I64 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = 32 + mCurrentInstruction.ShiftAmount();

		U64 r = a >> s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SLLV()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U64 r = a << (s & 0x1F);

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		} else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::DSLLV()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 a = getRegister(mCurrentInstruction.RegisterTarget());
		U64 s = getRegister(mCurrentInstruction.RegisterSource());

		U64 r = a << (s & 0x3F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SRLV()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U64 r = a >> (s & 0x1F);

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		} else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::DSRLV()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		U64 a = getRegister(mCurrentInstruction.RegisterTarget());
		U64 s = getRegister(mCurrentInstruction.RegisterSource());

		U64 r = a >> (s & 0x3F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SRAV()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U64 r = a >> (s & 0x1F);

		if (mCP0->is64BitMode()) {
			r = static_cast<I32>(static_cast<U32>(r));
		} else {
			r &= 0xFFFFFFFF;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::DSRAV()
	{
		if (mCP0->isReserved64BitInstruction()) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		I64 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U64 r = a >> (s & 0x3F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::BEQ()
	{
		mBranch = ESX_TRUE;
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = getRegister(mCurrentInstruction.RegisterTarget());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a == b) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BEQL()
	{
		BEQ();
		if (mTookBranch == ESX_FALSE) {
			mNullifyBranchSlot = ESX_TRUE;
		}
	}

	void VR4300::BNE()
	{
		mBranch = ESX_TRUE;
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = getRegister(mCurrentInstruction.RegisterTarget());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a != b) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BNEL()
	{
		BNE();
		if (mTookBranch == ESX_FALSE) {
			mNullifyBranchSlot = ESX_TRUE;
		}
	}

	void VR4300::BLTZ()
	{
		mBranch = ESX_TRUE;
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a < 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BLTZL()
	{
		BLTZ();
		if (mTookBranch == ESX_FALSE) {
			mNullifyBranchSlot = ESX_TRUE;
		}
	}

	void VR4300::BLTZAL()
	{
		mBranch = ESX_TRUE;
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		setRegister(GPRRegister::ra, mNextPC);

		if (a < 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BLTZALL()
	{
		BLTZAL();
		if (mTookBranch == ESX_FALSE) {
			mNullifyBranchSlot = ESX_TRUE;
		}
	}

	void VR4300::BLEZ()
	{
		mBranch = ESX_TRUE;
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a <= 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BLEZL()
	{
		BLEZ();
		if (mTookBranch == ESX_FALSE) {
			mNullifyBranchSlot = ESX_TRUE;
		}
	}

	void VR4300::BGTZ()
	{
		mBranch = ESX_TRUE;
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a > 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BGTZL()
	{
		BGTZ();
		if (mTookBranch == ESX_FALSE) {
			mNullifyBranchSlot = ESX_TRUE;
		}
	}

	void VR4300::BGEZ()
	{
		mBranch = ESX_TRUE;
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a >= 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BGEZL()
	{
		BGEZ();
		if (mTookBranch == ESX_FALSE) {
			mNullifyBranchSlot = ESX_TRUE;
		}
	}

	void VR4300::BGEZAL()
	{
		mBranch = ESX_TRUE;
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		setRegister(GPRRegister::ra, mNextPC);

		if (a >= 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BGEZALL()
	{
		BGEZAL();
		if (mTookBranch == ESX_FALSE) {
			mNullifyBranchSlot = ESX_TRUE;
		}
	}

	void VR4300::J()
	{
		U64 a = (mNextPC & 0xFFFFFFFFF0000000) | (mCurrentInstruction.PseudoAddress() << 2);
		mNextPC = a;
		mBranch = ESX_TRUE;
		mTookBranch = ESX_TRUE;
		mCallPC = mCurrentPC;
	}

	void VR4300::JR()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		mNextPC = a;
		mBranch = ESX_TRUE;
		mTookBranch = ESX_TRUE;
	}

	void VR4300::JAL()
	{
		setRegister(GPRRegister::ra, mNextPC);
		J();
	}

	void VR4300::JALR()
	{
		setRegister(mCurrentInstruction.RegisterDestination(), mNextPC);
		JR();
	}

	void VR4300::BREAK()
	{
		ESX_CORE_LOG_ERROR("0x{:08X} Break", mCurrentInstruction.Address);
		raiseException(ExceptionType::Breakpoint);
	}

	void VR4300::SYSCALL()
	{
		raiseException(ExceptionType::Syscall);
	}

	void VR4300::CACHE()
	{
		//TODO: All caching system
	}

	void VR4300::SYNC()
	{
		//NOP on VR4300
	}

	void VR4300::TGE()
	{
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I64 b = getRegister(mCurrentInstruction.RegisterTarget());

		if (a >= b) {
			raiseException(ExceptionType::Trap);
		}
	}

	void VR4300::TGEI()
	{
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I64 b = mCurrentInstruction.ImmediateSE();

		if (a >= b) {
			raiseException(ExceptionType::Trap);
		}
	}

	void VR4300::TGEU()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = getRegister(mCurrentInstruction.RegisterTarget());

		if (a >= b) {
			raiseException(ExceptionType::Trap);
		}
	}

	void VR4300::TGEIU()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.Immediate();

		if (a >= b) {
			raiseException(ExceptionType::Trap);
		}
	}

	void VR4300::TLT()
	{
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I64 b = getRegister(mCurrentInstruction.RegisterTarget());

		if (a < b) {
			raiseException(ExceptionType::Trap);
		}
	}

	void VR4300::TLTI()
	{
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I64 b = mCurrentInstruction.ImmediateSE();

		if (a < b) {
			raiseException(ExceptionType::Trap);
		}
	}

	void VR4300::TLTU()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = getRegister(mCurrentInstruction.RegisterTarget());

		if (a < b) {
			raiseException(ExceptionType::Trap);
		}
	}

	void VR4300::TLTIU()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.Immediate();

		if (a < b) {
			raiseException(ExceptionType::Trap);
		}
	}

	void VR4300::TEQ()
	{
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I64 b = getRegister(mCurrentInstruction.RegisterTarget());

		if (a == b) {
			raiseException(ExceptionType::Trap);
		}
	}

	void VR4300::TEQI()
	{
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I64 b = mCurrentInstruction.ImmediateSE();

		if (a == b) {
			raiseException(ExceptionType::Trap);
		}
	}

	void VR4300::TNE()
	{
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I64 b = getRegister(mCurrentInstruction.RegisterTarget());

		if (a != b) {
			raiseException(ExceptionType::Trap);
		}
	}

	void VR4300::TNEI()
	{
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I64 b = mCurrentInstruction.ImmediateSE();

		if (a != b) {
			raiseException(ExceptionType::Trap);
		}
	}

	void VR4300::COP0()
	{
		if (mCOPs[0]) {
			CoprocessorExecuteFunction coprocessorFunction = nullptr;
			if (mCurrentInstruction.RegisterSource().Value != 8) {
				coprocessorFunction = copDecodeRS[mCurrentInstruction.RegisterSource().Value];
			}
			else {
				coprocessorFunction = copDecodeBC[mCurrentInstruction.RegisterTarget().Value];
			}
			((mCOPs[0].get())->*coprocessorFunction)();
		}
	}

	void VR4300::COP1()
	{
		if (mCOPs[1]) {
			CoprocessorExecuteFunction coprocessorFunction = nullptr;
			if (mCurrentInstruction.RegisterSource().Value != 8) {
				coprocessorFunction = copDecodeRS[mCurrentInstruction.RegisterSource().Value];
			}
			else {
				coprocessorFunction = copDecodeBC[mCurrentInstruction.RegisterTarget().Value];
			}
			((mCOPs[1].get())->*coprocessorFunction)();
		}
	}

	void VR4300::COP2()
	{
		if (mCOPs[2]) {
			CoprocessorExecuteFunction coprocessorFunction = nullptr;
			if (mCurrentInstruction.RegisterSource().Value != 8) {
				coprocessorFunction = copDecodeRS[mCurrentInstruction.RegisterSource().Value];
			}
			else {
				coprocessorFunction = copDecodeBC[mCurrentInstruction.RegisterTarget().Value];
			}
			((mCOPs[2].get())->*coprocessorFunction)();
		}
	}

	void VR4300::COP3()
	{
		if (mCOPs[3]) {
			CoprocessorExecuteFunction coprocessorFunction = nullptr;
			if (mCurrentInstruction.RegisterSource().Value != 8) {
				coprocessorFunction = copDecodeRS[mCurrentInstruction.RegisterSource().Value];
			}
			else {
				coprocessorFunction = copDecodeBC[mCurrentInstruction.RegisterTarget().Value];
			}
			((mCOPs[3].get())->*coprocessorFunction)();
		}
	}

	void VR4300::LDC1()
	{
	}

	void VR4300::LDC2()
	{
	}

	void VR4300::LWC0()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::LWC1()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::LWC2()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::LWC3()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::SDC1()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::SDC2()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::SWC0()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::SWC1()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::SWC2()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::SWC3()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::NA()
	{
		raiseException(ExceptionType::ReservedInstruction);
	}

	void VR4300::iCacheStore(U32 address, U32 value)
	{
		U32 cacheLineNumber = address / 32;
		U32 wordAddress = (address & 0xF) / 4;

		InstructionCache& instruction = mICache.CacheLines[cacheLineNumber].Instructions[wordAddress];
		instruction.Word = value;
		mICache.CacheLines[cacheLineNumber].Valid = ESX_FALSE;
	}

	void VR4300::addWriteQueueOperation(const StoreOperation& writeOp)
	{
		if (writeOp.Address == 0x80) {
			ESX_CORE_LOG_ERROR("{:08x}h", mCurrentInstruction.Address);
		}
		mWriteQueue.push_back(writeOp);
	}

	void VR4300::doWriteQueueOperation(const StoreOperation& writeOp)
	{
		switch (writeOp.Size) {
			case sizeof(U8)  :  mRootBus->store<U8> (writeOp.Address, static_cast<U8> (writeOp.Data)); break;
			case sizeof(U16) :	mRootBus->store<U16>(writeOp.Address, static_cast<U16>(writeOp.Data)); break;
			case sizeof(U32) :	mRootBus->store<U32>(writeOp.Address, static_cast<U32>(writeOp.Data)); break;
		}
	}

	BIT VR4300::flushWriteQueue(U32 address)
	{
		auto foundIt = std::find_if(mWriteQueue.begin(), mWriteQueue.end(), [&](const StoreOperation& storeOp) { return storeOp.Address == address; });
		if (foundIt != mWriteQueue.end()) {
			doWriteQueueOperation(*foundIt);
			mWriteQueue.erase(foundIt);
			return ESX_TRUE;
		}
		return ESX_FALSE;
	}

	void VR4300::flushWriteQueueFirst()
	{
		if (mWriteQueue.size() == 0) return;
		auto it = mWriteQueue.begin();
		doWriteQueueOperation(*it);
		mWriteQueue.erase(it);
	}

	void VR4300::flushWriteQueueAll()
	{
		for (auto it = mWriteQueue.begin(); it != mWriteQueue.end(); ++it) {
			doWriteQueueOperation(*it);
		}
		mWriteQueue.clear();
	}

	void VR4300::raiseException(ExceptionType type, U32 virtualAddress) {
		mCP0->raiseException(type, virtualAddress);
	}
}
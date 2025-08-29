#include "VR4300.h"

#include <iostream>
#include <iomanip>
#include <fstream>

#include "Core/InterruptControl.h"

#include "optick.h"

namespace esx {

	VR4300::VR4300()
		:	BusDevice(ESX_TEXT("VR4300"))
	{
		reset();

	}

	VR4300::~VR4300()
	{
	}

	void VR4300::init()
	{

		mRootBus = getBus(ESX_TEXT("Root"));
		mInterruptControl = getBus("Root")->getDevice<InterruptControl>("InterruptControl");
	}

	void VR4300::clock()
	{
		if (mCyclesToWait == 0) {
			if (!mStall) {
				if (ADDRESS_UNALIGNED(mNextPC, U32)) {
					raiseException(ExceptionType::AddressErrorLoad);
				}

				U32 opcode = fetch(mPC);

				if (opcode != 0) {
					decode(mCurrentInstruction, opcode, mPC);
				}

				mCurrentPC = mPC;
				mPC = mNextPC;
				mNextPC += 4;

				mBranchSlot = mBranch;
				mTookBranchSlot = mTookBranch;
				mBranch = ESX_FALSE;
				mTookBranch = ESX_FALSE;

				if (opcode != 0 && mCurrentInstruction.Execute) {
					(this->*mCurrentInstruction.Execute)();
				}

				mRegisters[mMemoryLoad.first] = mMemoryLoad.second;
				mRegisters[0] = 0;
				mMemoryLoad = mPendingLoad;
				resetPendingLoad();

				mRegisters[mWriteBack.first] = mWriteBack.second;
				mWriteBack.first = RegisterIndex(0);
				mRegisters[0] = 0;
			}
			mCyclesToWait = 2;
		}
		mCyclesToWait--;

		mInterruptControl->clock(mCycles);
		handleInterrupts();

		mCycles++;
	}


	U32 VR4300::fetch(U32 address)
	{
		if (ADDRESS_UNALIGNED(address, U32)) {
			raiseException(ExceptionType::AddressErrorLoad);
			return 0;
		}

		if (isCacheActive(address)) {
			U32 index = (address >> 2) & 0x7;
			U32 cacheLineNumber = (address >> 5) & 0x1FF;
			U32 tag = address >> 12;

			auto& cacheLine = mICache.CacheLines[cacheLineNumber];
			if (cacheLine.Tag == tag && cacheLine.Valid) {
				auto& instruction = cacheLine.Instructions[index];
				return instruction.Word;
			} else {
				return cacheMiss(address, cacheLineNumber, tag, index);
			}
		} else {
			return mRootBus->load<U32>(address);
		}
	}

	static const Array<ExecuteFunction, 64> primaryOpCodeDecode = {
		&VR4300::NA,	&VR4300::NA,		&VR4300::J,		&VR4300::JAL,	&VR4300::BEQ,	&VR4300::BNE,	&VR4300::BLEZ,	&VR4300::BGTZ,
		&VR4300::ADDI,	&VR4300::ADDIU,		&VR4300::SLTI,	&VR4300::SLTIU, &VR4300::ANDI,	&VR4300::ORI,	&VR4300::XORI,	&VR4300::LUI,
		&VR4300::COP0,	&VR4300::COP1,		&VR4300::COP2,	&VR4300::NA,	&VR4300::BEQL,	&VR4300::BNEL,	&VR4300::BLEZL,	&VR4300::BGTZL,
		&VR4300::DADDI,	&VR4300::DADDIU,	&VR4300::LDL,	&VR4300::LDR,	&VR4300::NA,	&VR4300::NA,	&VR4300::NA,	&VR4300::NA,
		&VR4300::LB,	&VR4300::LH,		&VR4300::LWL,	&VR4300::LW,	&VR4300::LBU,	&VR4300::LHU,	&VR4300::LWR,	&VR4300::LWU,
		&VR4300::SB,	&VR4300::SH,		&VR4300::SWL,	&VR4300::SW,	&VR4300::SDL,	&VR4300::SDR,	&VR4300::SWR,	&VR4300::CACHE,
		&VR4300::LL,	&VR4300::LWC1,		&VR4300::LWC2,	&VR4300::NA,	&VR4300::LLD,	&VR4300::LDC1,	&VR4300::LDC2,	&VR4300::LD,
		&VR4300::SC,	&VR4300::SWC1,		&VR4300::SWC2,	&VR4300::NA,	&VR4300::SCD,	&VR4300::SDC1,	&VR4300::SDC2,	&VR4300::SD
	};

	static const Array<ExecuteFunction, 64> secondaryOpCodeDecode = {
		&VR4300::SLL,	&VR4300::NA,	&VR4300::SRL,	&VR4300::SRA,	&VR4300::SLLV,		&VR4300::NA,		&VR4300::SRLV,		&VR4300::SRAV,
		&VR4300::JR,	&VR4300::JALR,	&VR4300::NA,	&VR4300::NA,	&VR4300::SYSCALL,	&VR4300::BREAK,		&VR4300::NA,		&VR4300::SYNC,
		&VR4300::MFHI,	&VR4300::MTHI,	&VR4300::MFLO,	&VR4300::MTLO,	&VR4300::DSLLV,		&VR4300::NA,		&VR4300::DSRLV,		&VR4300::DSRAV,
		&VR4300::MULT,	&VR4300::MULTU, &VR4300::DIV,	&VR4300::DIVU,	&VR4300::DMULT,		&VR4300::DMULTU,	&VR4300::DDIV,		&VR4300::DDIVU,
		&VR4300::ADD,	&VR4300::ADDU,	&VR4300::SUB,	&VR4300::SUBU,	&VR4300::AND,		&VR4300::OR,		&VR4300::XOR,		&VR4300::NOR,
		&VR4300::NA,	&VR4300::NA,	&VR4300::SLT,	&VR4300::SLTU,	&VR4300::DADD,		&VR4300::DADDU,		&VR4300::DSUB,		&VR4300::DSUBU,
		&VR4300::TGE,	&VR4300::TGEU,	&VR4300::TLT,	&VR4300::TLTU,	&VR4300::TEQ,		&VR4300::NA,		&VR4300::TNE,		&VR4300::NA,
		&VR4300::DSLL,	&VR4300::NA,	&VR4300::DSRL,	&VR4300::DSRA,	&VR4300::DSLL32,	&VR4300::NA,		&VR4300::DSRL32,	&VR4300::DSRA32
	};

	static const Array<ExecuteFunction, 32> branchOpCodeDecode = {
		&VR4300::BLTZ,		&VR4300::BGEZ,		&VR4300::BLTZL,		&VR4300::BGEZL,		&VR4300::NA,	&VR4300::NA,	&VR4300::NA,	&VR4300::NA,
		&VR4300::TGEI,		&VR4300::TGEIU,		&VR4300::TLTI,		&VR4300::TLTIU,		&VR4300::TEQI,	&VR4300::NA,	&VR4300::TNEI,	&VR4300::NA,
		&VR4300::BLTZAL,	&VR4300::BGEZAL,	&VR4300::BLTZALL,	&VR4300::BGEZALL,	&VR4300::NA,	&VR4300::NA,	&VR4300::NA,	&VR4300::NA,
		&VR4300::NA,		&VR4300::NA,		&VR4300::NA,		&VR4300::NA,		&VR4300::NA,	&VR4300::NA,	&VR4300::NA,	&VR4300::NA
	};

	void VR4300::decode(Instruction& result, U32 instruction, U32 address, BIT suppressException)
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

	void VR4300::reset()
	{
		mRegisters = {};
		mCP0 = {};

		mMemoryLoad = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
		mPendingLoad = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
		mWriteBack = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);

		mPC = 0;
		mNextPC = 0;
		mCurrentPC = 0;
		mCallPC = 0;
		mHI = 0;
		mLO = 0;
		
		std::memset(&mICache, 0, sizeof(iCache));

		mStall = ESX_FALSE;

		mBranch = ESX_FALSE;
		mBranchSlot = ESX_FALSE;
		mTookBranch = ESX_FALSE;
		mTookBranchSlot = ESX_FALSE;

		mGPUClock = 0;

		mCycles = 0;

		mPC = 0xBFC00000;
		mNextPC = mPC + 4;
		setCP0Register(COP0Register::PRId, 0x00000002);
		resetPendingLoad();

		mMemoryLoad = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
		mPendingLoad = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
		mWriteBack = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
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

		U32 r = a + b;

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

		U32 r = a - b;

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

		U32 r = a + b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::MULT()
	{
		I64 a = static_cast<I64>(static_cast<I32>(getRegister(mCurrentInstruction.RegisterSource())));
		I64 b = static_cast<I64>(static_cast<I32>(getRegister(mCurrentInstruction.RegisterTarget())));

		U64 r = static_cast<U64>(a * b);

		mHI = (r >> 32) & 0xFFFFFFFF;
		mLO = r & 0xFFFFFFFF;
	}

	void VR4300::MULTU()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = getRegister(mCurrentInstruction.RegisterTarget());

		U64 r = a * b;

		mHI = (r >> 32) & 0xFFFFFFFF;
		mLO = r & 0xFFFFFFFF;
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
				mLO = 0xFFFFFFFF;
			} else {
				mLO = 1;
			}
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
			mLO = 0xFFFFFFFF;
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

	void VR4300::LW()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LW from {:08x} not handled", m);
			return;
		}

		U32 r = load<U32>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LH()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LH from {:08x} not handled", m);
			return;
		}

		U32 r = load<U16>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		} else {
			r = static_cast<U32>(static_cast<I32>(static_cast<I16>(static_cast<U16>(r))));
		}

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LHU()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LHU from {:08x} not handled", m);
			return;
		}

		U32 r = load<U16>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LB()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LB from {:08x} not handled", m);
			return;
		}

		U32 r = load<U8>(m, exception);
		r = static_cast<U32>(static_cast<I32>(static_cast<I8>(static_cast<U8>(r))));

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LBU()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LBU from {:08x} not handled", m);
			return;
		}

		U32 r = load<U8>(m, exception);

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::LWL()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 c = getRegister(mCurrentInstruction.RegisterTarget());
		if (mMemoryLoad.first == mCurrentInstruction.RegisterTarget()) {
			c = mMemoryLoad.second;
		}

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LWL from {:08x} not handled", m);
			return;
		}

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am, exception);

		U32 u = m & 0x3;
		U32 r = (c & (0x00FFFFFF >> (u * 8))) | (aw << (24 - (u * 8)));

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::SWL()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 c = getRegister(mCurrentInstruction.RegisterTarget());

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated SWL from {:08x} not handled", m);
			return;
		}

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am, exception);

		U32 u = (m & 0x3) * 8;
		U32 mr = (aw & (0xFFFFFF00 << u)) | (c >> (24 - u));

		store<U32>(am, mr);
	}


	void VR4300::LWR()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 c = getRegister(mCurrentInstruction.RegisterTarget());
		if (mMemoryLoad.first == mCurrentInstruction.RegisterTarget()) {
			c = mMemoryLoad.second;
		}

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LWR from {:08x} not handled", m);
			return;
		}

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am, exception);

		U32 u = m & 0x3;
		U32 r = (c & (0xFFFFFF00 << ((0x3 - u) * 8))) | (aw >> (u * 8));

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::SWR()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 c = getRegister(mCurrentInstruction.RegisterTarget());

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated SWR to {:08x} not handled", m);
			return;
		}

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am, exception);

		U32 u = m & 0x3;
		U32 mr = (aw & (0x00FFFFFF >> ((0x3 - u) * 8))) | (c << (u * 8));

		store<U32>(am, mr);
	}

	void VR4300::SB()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 v = getRegister(mCurrentInstruction.RegisterTarget());

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache SB store to {:08x} not handled", m);
			return;
		}
		
		store<U8>(m, v);
	}

	void VR4300::SH()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 v = getRegister(mCurrentInstruction.RegisterTarget());

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache SH store to {:08x} not handled", m);
			return;
		}

		store<U16>(m, v);
	}

	void VR4300::SW()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		U32 v = getRegister(mCurrentInstruction.RegisterTarget());

		if ((sr & 0x10000) != 0) {
			iCacheStore(m, v);
			return;
		}


		store<U32>(m, v);
	}

	void VR4300::LUI()
	{
		U32 r = mCurrentInstruction.Immediate() << 16;
		setRegister(mCurrentInstruction.RegisterTarget(), r);
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

		U32 r = a << s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SRL()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U32 r = a >> s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SRA()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U32 r = a >> s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SLLV()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U32 r = a << (s & 0x1F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SRLV()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U32 r = a >> (s & 0x1F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::SRAV()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U32 r = a >> (s & 0x1F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::BEQ()
	{
		//ESX_CORE_LOG_TRACE("BEQ {:08x}h", mCurrentInstruction.Address);

		mBranch = ESX_TRUE;
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a == b) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BNE()
	{
		mBranch = ESX_TRUE;
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a != b) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BLTZ()
	{
		mBranch = ESX_TRUE;
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a < 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BLTZAL()
	{
		mBranch = ESX_TRUE;
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		setRegister(GPRRegister::ra, mNextPC);

		if (a < 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BLEZ()
	{
		mBranch = ESX_TRUE;
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a <= 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BGTZ()
	{
		mBranch = ESX_TRUE;
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a > 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BGEZ()
	{
		mBranch = ESX_TRUE;
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a >= 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BGEZAL()
	{
		mBranch = ESX_TRUE;
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		setRegister(GPRRegister::ra, mNextPC);

		if (a >= 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::J()
	{
		U32 a = (mNextPC & 0xF0000000) | (mCurrentInstruction.PseudoAddress() << 2);
		mNextPC = a;
		mBranch = ESX_TRUE;
		mTookBranch = ESX_TRUE;
		mCallPC = mCurrentPC;
	}

	void VR4300::JR()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
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

	static const Array<CoprocessorExecuteFunction, 32> cop0decode = {
		&SystemControlCoprocessor::MF,	&SystemControlCoprocessor::DMF,	&SystemControlCoprocessor::CF,	&SystemControlCoprocessor::NA,	&SystemControlCoprocessor::MT,	&SystemControlCoprocessor::DMT,	&SystemControlCoprocessor::CT,	&SystemControlCoprocessor::NA,
		&SystemControlCoprocessor::NA,	&SystemControlCoprocessor::NA,	&SystemControlCoprocessor::NA,	&SystemControlCoprocessor::NA,	&SystemControlCoprocessor::NA,	&SystemControlCoprocessor::NA,	&SystemControlCoprocessor::NA,	&SystemControlCoprocessor::NA,
		&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO,
		&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO,	&SystemControlCoprocessor::CO
	};

	void VR4300::COP0()
	{
		CoprocessorExecuteFunction coprocessorFunction = cop0decode[mCurrentInstruction.RegisterSource().Value];
		((&mCP0)->*coprocessorFunction)(this);
	}

	void VR4300::COP1()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::COP2()
	{
		/*//ESX_CORE_LOG_TRACE("COP2");
		if (CO(mCurrentInstruction.binaryInstruction) == 0) {
			switch (mCurrentInstruction.RegisterSource().Value) {
				case 0x00: {
					MFC2();
					break;
				}
				case 0x02: {
					CFC2();
					break;
				}
				case 0x04: {
					MTC2();
					break;
				}
				case 0x06: {
					CTC2();
					break;
				}
				case 0x08: {
					if (mCurrentInstruction.RegisterTarget().Value == 1) {
						BC2T();
					}
					else if (mCurrentInstruction.RegisterTarget().Value == 0) {
						BC2F();
					}
					break;
				}
				default: {
					raiseException(ExceptionType::CoprocessorUnusable);
					break;
				}
			}
		}
		else {
			//mGTE.command(mCurrentInstruction.Immediate25());
		}*/
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::COP3()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::MTC0()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 r = getRegister(mCurrentInstruction.RegisterTarget());

		if (mCurrentInstruction.RegisterDestination() <= 2 ||
			mCurrentInstruction.RegisterDestination() == 4 ||
			mCurrentInstruction.RegisterDestination() == 10 ||
			(mCurrentInstruction.RegisterDestination() >= 32 && mCurrentInstruction.RegisterDestination() <= 63)) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		if (mCurrentInstruction.RegisterDestination() < 16 && (sr & 0x10000002) == 0x1) {
			raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		switch ((COP0Register)(U8)mCurrentInstruction.RegisterDestination()) {
			case COP0Register::Cause: {
				U32 t = getCP0Register(mCurrentInstruction.RegisterDestination());

				t &= ~0x300;
				r &= 0x300;

				r |= t;
				break;
			}

			case COP0Register::EPC:
			case COP0Register::BadVAddr:
			case COP0Register::PRId:
			case COP0Register::JumpDest:
				return;

		}

		setCP0Register(mCurrentInstruction.RegisterDestination(), r);
	}

	void VR4300::MFC0()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 r = getCP0Register(mCurrentInstruction.RegisterDestination());

		if (mCurrentInstruction.RegisterDestination() < 16 && (sr & 0x10000002) == 0x1) {
			raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void VR4300::CFC2()
	{
		//addPendingLoad(mCurrentInstruction.RegisterTarget(), mGTE.getRegister(32 + mCurrentInstruction.RegisterDestination().Value));
	}

	void VR4300::MTC2()
	{
		//mGTE.setRegister(mCurrentInstruction.RegisterDestination().Value, getRegister(mCurrentInstruction.RegisterTarget()));
	}

	void VR4300::MFC2()
	{
		//addPendingLoad(mCurrentInstruction.RegisterTarget(), mGTE.getRegister(mCurrentInstruction.RegisterDestination().Value));
	}

	void VR4300::CTC2()
	{
		//mGTE.setRegister(32 + mCurrentInstruction.RegisterDestination().Value, getRegister(mCurrentInstruction.RegisterTarget()));
	}

	void VR4300::BC0F()
	{
		mBranch = ESX_TRUE;
		U8 a = getCP0Register(COP0Register::SR) & (1 << 6);
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a == 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BC2F()
	{
		/*mBranch = ESX_TRUE;
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (mGTE.getFlag() == false) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}*/
	}

	void VR4300::BC0T()
	{
		mBranch = ESX_TRUE;
		U8 a = getCP0Register(COP0Register::SR) & (1 << 6);
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a == 1) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void VR4300::BC2T()
	{
		mBranch = ESX_TRUE;
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		/*if (mGTE.getFlag() == true) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}*/
	}

	void VR4300::RFE()
	{
		U32 sr = getCP0Register(COP0Register::SR);

		if ((sr & 0x10000002) == 0x1) {
			raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		U32 mode = sr & 0x3F;
		sr &= ~0xF;
		sr |= (mode >> 2) & 0x3F;

		setCP0Register(COP0Register::SR, sr);
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
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated load from {:08x} not handled", m);
			return;
		}

		U32 r = load<U32>(m, exception);

		//mGTE.setRegister(mCurrentInstruction.RegisterTarget().Value, r);
	}

	void VR4300::LWC3()
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
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated store to {:08x} not handled", m);
			return;
		}

		//U32 v = mGTE.getRegister(mCurrentInstruction.RegisterTarget().Value);

		//store<U32>(m, v);
	}

	void VR4300::SWC3()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void VR4300::NA()
	{
		raiseException(ExceptionType::ReservedInstruction);
	}

	void VR4300::addPendingLoad(RegisterIndex index, U64 value)
	{
		mPendingLoad.first = index;
		mPendingLoad.second = value;

		if (mMemoryLoad.first == index) {
			mMemoryLoad = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
		}
	}

	void VR4300::resetPendingLoad()
	{
		mPendingLoad.first = RegisterIndex(0);
		mPendingLoad.second = 0;
	}

	void VR4300::setRegister(RegisterIndex index, U64 value)
	{
		mWriteBack.first = index;
		mWriteBack.second = value;
	}

	U64 VR4300::getRegister(RegisterIndex index)
	{
		return mRegisters[index.Value];
	}

	void VR4300::setCP0Register(RegisterIndex index, U32 value)
	{
		mCP0Registers[index.Value] = value;
	}

	U32 VR4300::getCP0Register(RegisterIndex index)
	{
		return mCP0Registers[index.Value];
	}

	U32 VR4300::cacheMiss(U32 address, U32 cacheLineNumber, U32 tag, U32 startIndex)
	{
		auto& cacheLine = mICache.CacheLines[cacheLineNumber];

		U32 baseAddr = address & ~0x1F;

		for (U32 index = 0; index < cacheLine.Instructions.size(); index++) {
			auto& instruction = cacheLine.Instructions[index];
			instruction.Word = mRootBus->load<U32>(baseAddr + index * sizeof(U32));
		}

		cacheLine.Tag = tag;
		cacheLine.Valid = ESX_TRUE;

		return cacheLine.Instructions[startIndex].Word;
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

	void VR4300::handleInterrupts()
	{
		U32 cause = getCP0Register(COP0Register::Cause);
		U32 sr = getCP0Register(COP0Register::SR);

		U32 opcodeNextInstruction = fetch(mPC);
		if ((opcodeNextInstruction >> 26) != 0x12) {
			if (mInterruptControl->interruptPending()) {
				cause |= (1 << 10);
			}
			else {
				cause &= ~(1 << 10);
			}

			setCP0Register(COP0Register::Cause, cause);

			BIT IEC = sr & 0x1;
			U8 IM = (sr >> 8) & 0xFF;
			U8 IP = (cause >> 8) & 0xFF;

			if (IEC && ((IM & IP) > 0)) {
				raiseException(ExceptionType::Interrupt);
			}
		}
	}

	void VR4300::raiseException(ExceptionType type)
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 epc = getCP0Register(COP0Register::EPC);
		U32 cause = getCP0Register(COP0Register::Cause);

		U32 handler = 0x80000080;
		if ((sr & (1 << 22)) != 0) {
			handler = 0xBFC00180;
		}

		U32 mode = sr & 0x3F;
		sr &= ~0x3F;
		sr |= (mode << 2) & 0x3F;

		cause = ((U32)type) << 2;

		if (type == ExceptionType::Interrupt) {
			epc = mPC;
			mBranchSlot = mBranch;
			mTookBranchSlot = mTookBranch;
		} else {
			epc = mCurrentPC;
		}

		if (mBranchSlot) {
			epc -= 4;
			cause |= 1 << 31;
			setCP0Register(COP0Register::JumpDest, mPC);

			if (mTookBranchSlot) {
				cause |= 1 << 30;
			}
		}

		setCP0Register(COP0Register::Cause, cause);
		setCP0Register(COP0Register::EPC, epc);
		setCP0Register(COP0Register::SR, sr);

		mPC = handler;
		mNextPC = mPC + 4;
	}

	String Instruction::Mnemonic(const SharedPtr<VR4300>& cpuState) const
	{
		constexpr static std::array<StringView,32> registersMnemonics = {
			ESX_TEXT("$zero"),
			ESX_TEXT("$at"),
			ESX_TEXT("$v0"),ESX_TEXT("$v1"),
			ESX_TEXT("$a0"),ESX_TEXT("$a1"),ESX_TEXT("$a2"),ESX_TEXT("$a3"),
			ESX_TEXT("$t0"),ESX_TEXT("$t1"),ESX_TEXT("$t2"),ESX_TEXT("$t3"),ESX_TEXT("$t4"),ESX_TEXT("$t5"),ESX_TEXT("$t6"),ESX_TEXT("$t7"),
			ESX_TEXT("$s0"),ESX_TEXT("$s1"),ESX_TEXT("$s2"),ESX_TEXT("$s3"),ESX_TEXT("$s4"),ESX_TEXT("$s5"),ESX_TEXT("$s6"),ESX_TEXT("$s7"),
			ESX_TEXT("$t8"),ESX_TEXT("$t9"),
			ESX_TEXT("$k0"),ESX_TEXT("$k1"),
			ESX_TEXT("$gp"),
			ESX_TEXT("$sp"),
			ESX_TEXT("$fp"),
			ESX_TEXT("$ra")
		};

		switch (Opcode()) {
			//R Type
			case 0x00: {
				switch (Function()) {
					case 0x00: {
						return FormatString(ESX_TEXT("sll {},{},0x{:02x}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], ShiftAmount());
					}
					case 0x02: {
						return FormatString(ESX_TEXT("srl {},{},0x{:02x}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], ShiftAmount());
					}
					case 0x03: {
						return FormatString(ESX_TEXT("sra {},{},0x{:02x}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], ShiftAmount());
					}
					case 0x04: {
						return FormatString(ESX_TEXT("sllv {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x06: {
						return FormatString(ESX_TEXT("srlv {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x07: {
						return FormatString(ESX_TEXT("srav {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x08: {
						return FormatString(ESX_TEXT("jr {}"), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x09: {
						return FormatString(ESX_TEXT("jalr {},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x0C: {
						return FormatString(ESX_TEXT("syscall"));
					}
					case 0x0D: {
						return FormatString(ESX_TEXT("break"));
					}
					case 0x10: {
						return FormatString(ESX_TEXT("mfhi {}"), registersMnemonics[(U8)RegisterDestination()]);
					}
					case 0x11: {
						return FormatString(ESX_TEXT("mthi {}"), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x12: {
						return FormatString(ESX_TEXT("mflo {}"), registersMnemonics[(U8)RegisterDestination()]);
					}
					case 0x13: {
						return FormatString(ESX_TEXT("mtlo {}"), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x18: {
						return FormatString(ESX_TEXT("mult {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x19: {
						return FormatString(ESX_TEXT("multu {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x1A: {
						return FormatString(ESX_TEXT("div {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x1B: {
						return FormatString(ESX_TEXT("divu {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x20: {
						if (RegisterTarget().Value == 0) {
							return FormatString(ESX_TEXT("move {},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()]);
						}
						else {
							return FormatString(ESX_TEXT("add {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
						}
					}
					case 0x21: {
						if (RegisterTarget().Value == 0) {
							return FormatString(ESX_TEXT("move {},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()]);
						}
						else {
							return FormatString(ESX_TEXT("addu {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
						}
					}
					case 0x22: {
						return FormatString(ESX_TEXT("sub {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x23: {
						return FormatString(ESX_TEXT("subu {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x24: {
						return FormatString(ESX_TEXT("and {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x25: {
						return FormatString(ESX_TEXT("or {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x26: {
						return FormatString(ESX_TEXT("xor {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x27: {
						return FormatString(ESX_TEXT("nor {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x2A: {
						return FormatString(ESX_TEXT("slt {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x2B: {
						return FormatString(ESX_TEXT("sltu {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
				}

				break;
			}

			//J Type
			case 0x02: {
				return FormatString(ESX_TEXT("j 0x{:08x}"), ((Address + 4) & 0xF0000000) | (PseudoAddress() << 2));
			}
			case 0x03: {
				return FormatString(ESX_TEXT("jal 0x{:08x}"), ((Address + 4) & 0xF0000000) | (PseudoAddress() << 2));
			}

			default: {
				switch (Opcode()) {
					case 0x01: {
						switch (RegisterTarget().Value) {
							case 0x00: {
								return FormatString(ESX_TEXT("bltz {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
							}
							case 0x01: {
								return FormatString(ESX_TEXT("bgez {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
							}
							case 0x10: {
								return FormatString(ESX_TEXT("bltzal {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
							}
							case 0x11: {
								return FormatString(ESX_TEXT("bgezal {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
							}
						}

						break;
					}
					case 0x04: {
						return FormatString(ESX_TEXT("beq {},{},0x{:08x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
					}
					case 0x05: {
						return FormatString(ESX_TEXT("bne {},{},0x{:08x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
					}
					case 0x06: {
						return FormatString(ESX_TEXT("blez {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
					}
					case 0x07: {
						return FormatString(ESX_TEXT("bgtz {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
					}
					case 0x08: {
						return FormatString(ESX_TEXT("addi {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (I16)Immediate());
					}
					case 0x09: {
						return FormatString(ESX_TEXT("addiu {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (I16)Immediate());
					}
					case 0x0A: {
						return FormatString(ESX_TEXT("slti {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (I16)Immediate());
					}
					case 0x0B: {
						return FormatString(ESX_TEXT("sltiu {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (I16)Immediate());
					}
					case 0x0C: {
						return FormatString(ESX_TEXT("andi {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], Immediate());
					}
					case 0x0D: {
						return FormatString(ESX_TEXT("ori {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], Immediate());
					}
					case 0x0E: {
						return FormatString(ESX_TEXT("xori {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], Immediate());
					}
					case 0x0F: {
						return FormatString(ESX_TEXT("lui {},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], Immediate());
					}
					case 0x10:
					case 0x11:
					case 0x12:
					case 0x13: {
						U8 cpn = CO_N(binaryInstruction);
						if (CO(binaryInstruction) == 0) {
							switch (RegisterSource().Value) {
								case 0x00: {
									return FormatString(ESX_TEXT("mfc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x02: {
									return FormatString(ESX_TEXT("cfc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x04: {
									return FormatString(ESX_TEXT("mtc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x06: {
									return FormatString(ESX_TEXT("ctc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x08: {
									switch (RegisterTarget()) {
										case 0: return FormatString(ESX_TEXT("bc{}f 0x{:04x}"), cpn, Immediate());
										case 1: return FormatString(ESX_TEXT("bc{}t 0x{:04x}"), cpn, Immediate());
									}
								}
							}
						}
						else {
							switch (cpn) {
								case 0: {
									switch (COP_FUNC(binaryInstruction)) {
										case 0x10: {
											return FormatString(ESX_TEXT("rfe"));
										}
									}
									break;
								}

								case 2: {
									return FormatString(ESX_TEXT("cop2 0x{:08x}"), Immediate25());
								}
							}
						}
						break;
					}
					case 0x20: {
						return FormatString(ESX_TEXT("lb {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x21: {
						return FormatString(ESX_TEXT("lh {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x22: {
						return FormatString(ESX_TEXT("lwl {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x23: {
						return FormatString(ESX_TEXT("lw {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x24: {
						return FormatString(ESX_TEXT("lbu {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x25: {
						return FormatString(ESX_TEXT("lhu {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x26: {
						return FormatString(ESX_TEXT("lwr {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x28: {
						return FormatString(ESX_TEXT("sb {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x29: {
						return FormatString(ESX_TEXT("sh {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x2A: {
						return FormatString(ESX_TEXT("swl {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x2B: {
						return FormatString(ESX_TEXT("sw {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x2E: {
						return FormatString(ESX_TEXT("swr {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}

					case 0x30:
					case 0x31:
					case 0x32:
					case 0x33: {
						U8 cpn = CO_N(binaryInstruction);
						return FormatString(ESX_TEXT("lwc{} ${},0x{:08x}"), cpn, registersMnemonics[(U8)RegisterTarget()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
						break;
					}


					case 0x38:
					case 0x39:
					case 0x3A:
					case 0x3B: {
						U8 cpn = CO_N(binaryInstruction);
						return FormatString(ESX_TEXT("swc{} ${},0x{:08x}"), cpn, registersMnemonics[(U8)RegisterTarget()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
						break;
					}
				}
			}
		}

		return ESX_TEXT("");
	}

}
#include "R4000.h"

#include <iostream>
#include <iomanip>
#include <fstream>

#include "optick.h"

namespace esx {

	R4000::R4000()
		:	MIPSProcessor(ESX_TEXT("R4000"))
	{
	}

	R4000::~R4000()
	{
	}

	void R4000::init()
	{

		mRootBus = getBus(ESX_TEXT("Root"));
	}

	void R4000::clock()
	{
		MIPSProcessor::clock();

		mCurrentPC &= 0xFFF;
		mPC &= 0xFFF;
		mNextPC &= 0xFFF;
	}

	U32 R4000::fetch(U32 virtualAddress)
	{
		BIT cached = ESX_FALSE;
		U32 physicalAddress = 0x04001000 + (virtualAddress & 0xFFF);

		if (cached) {
			return accessCache(mICache, virtualAddress, physicalAddress);
		} else {
			return mRootBus->load(physicalAddress);
		}
	}

	static const Array<R4000ExecuteFunction, 64> primaryOpCodeDecode = {
		&R4000::NA,		&R4000::NA,		&R4000::J,		&R4000::JAL,	&R4000::BEQ,	&R4000::BNE,	&R4000::BLEZ,	&R4000::BGTZ,
		&R4000::ADDI,	&R4000::ADDIU,		&R4000::SLTI,	&R4000::SLTIU, &R4000::ANDI,	&R4000::ORI,	&R4000::XORI,	&R4000::LUI,
		&R4000::COP0,	&R4000::COP1,		&R4000::COP2,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,
		&R4000::NA,		&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,
		&R4000::LB,		&R4000::LH,		&R4000::NA,	&R4000::LW,	&R4000::LBU,	&R4000::LHU,	&R4000::NA,	&R4000::NA,
		&R4000::SB,		&R4000::SH,		&R4000::NA,	&R4000::SW,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::CACHE,
		&R4000::LL,		&R4000::LWC1,		&R4000::LWC2,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,
		&R4000::SC,		&R4000::SWC1,		&R4000::SWC2,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA
	};

	static const Array<R4000ExecuteFunction, 64> secondaryOpCodeDecode = {
		&R4000::SLL,	&R4000::NA,	&R4000::SRL,	&R4000::SRA,	&R4000::SLLV,		&R4000::NA,		&R4000::SRLV,		&R4000::SRAV,
		&R4000::JR,	&R4000::JALR,	&R4000::NA,	&R4000::NA,	&R4000::SYSCALL,	&R4000::BREAK,		&R4000::NA,		&R4000::SYNC,
		&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,		&R4000::NA,		&R4000::NA,		&R4000::NA,
		&R4000::NA,	&R4000::NA, &R4000::NA,	&R4000::NA,	&R4000::NA,		&R4000::NA,	&R4000::NA,		&R4000::NA,
		&R4000::ADD,	&R4000::ADDU,	&R4000::SUB,	&R4000::SUBU,	&R4000::AND,		&R4000::OR,		&R4000::XOR,		&R4000::NOR,
		&R4000::NA,	&R4000::NA,	&R4000::SLT,	&R4000::SLTU,	&R4000::NA,		&R4000::NA,		&R4000::NA,		&R4000::NA,
		&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,		&R4000::NA,		&R4000::NA,		&R4000::NA,
		&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,		&R4000::NA,	&R4000::NA
	};

	static const Array<R4000ExecuteFunction, 32> branchOpCodeDecode = {
		&R4000::BLTZ,		&R4000::BGEZ,		&R4000::NA,		&R4000::NA,		&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,
		&R4000::NA,		&R4000::NA,		&R4000::NA,		&R4000::NA,		&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,
		&R4000::BLTZAL,	&R4000::BGEZAL,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA,
		&R4000::NA,		&R4000::NA,		&R4000::NA,		&R4000::NA,		&R4000::NA,	&R4000::NA,	&R4000::NA,	&R4000::NA
	};

	void R4000::decode(R4000Instruction& result, U32 instruction, U32 address, BIT suppressException)
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

	void R4000::execute(R4000Instruction& instruction)
	{
		(this->*instruction.Execute)();
	}

	void R4000::reset()
	{
		MIPSProcessor::reset();
		mPC = 0x000;
		mNextPC = mPC + 4;
	}

	void R4000::ADD()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = getRegister(mCurrentInstruction.RegisterTarget());

		I32 r = a + b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::ADDU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U64 r = a + b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::SUB()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = getRegister(mCurrentInstruction.RegisterTarget());

		I32 r = a - b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::SUBU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U64 r = a - b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::ADDI()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = mCurrentInstruction.ImmediateSE();

		I32 r = a + b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::ADDIU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U64 r = a + b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::LW()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 r = load<U32>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::LH()
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
		}

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::LHU()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U32 r = load<U16>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::LB()
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
		}

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::LBU()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U32 r = load<U8>(m, exception);

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::SB()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 v = getRegister(mCurrentInstruction.RegisterTarget());

		U64 m = a + b;

		store<U8>(m, v);
	}

	void R4000::SH()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 v = getRegister(mCurrentInstruction.RegisterTarget());

		U64 m = a + b;

		store<U16>(m, v);
	}

	void R4000::SC()
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

	void R4000::SW()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 v = getRegister(mCurrentInstruction.RegisterTarget());

		store<U32>(m, v);
	}

	void R4000::LUI()
	{
		U64 r = mCurrentInstruction.Immediate() << 16;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::LL()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 r = load<U32>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
		mLLBit = ESX_TRUE;
	}

	void R4000::SLT()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a < b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::SLTU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a < b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::SLTI()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = mCurrentInstruction.ImmediateSE();

		U32 r = a < b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::SLTIU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 r = a < b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::AND()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a & b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::ANDI()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.Immediate();

		U32 r = a & b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::OR()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a | b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::ORI()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.Immediate();

		U32 r = a | b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::XOR()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a ^ b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::XORI()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.Immediate();

		U32 r = a ^ b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::NOR()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = ~(a | b);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::SLL()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U64 r = a << s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::SRL()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U64 r = a >> s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::SRA()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U64 r = a >> s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::SLLV()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U64 r = a << (s & 0x1F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::SRLV()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U64 r = a >> (s & 0x1F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::SRAV()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U64 r = a >> (s & 0x1F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::BEQ()
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

	void R4000::BNE()
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

	void R4000::BLTZ()
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

	void R4000::BLTZAL()
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

	void R4000::BLEZ()
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

	void R4000::BGTZ()
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

	void R4000::BGEZ()
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

	void R4000::BGEZAL()
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

	void R4000::J()
	{
		U64 a = (mNextPC & 0xFFFFFFFFF0000000) | (mCurrentInstruction.PseudoAddress() << 2);
		mNextPC = a;
		mBranch = ESX_TRUE;
		mTookBranch = ESX_TRUE;
		mCallPC = mCurrentPC;
	}

	void R4000::JR()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		mNextPC = a;
		mBranch = ESX_TRUE;
		mTookBranch = ESX_TRUE;
	}

	void R4000::JAL()
	{
		setRegister(GPRRegister::ra, mNextPC);
		J();
	}

	void R4000::JALR()
	{
		setRegister(mCurrentInstruction.RegisterDestination(), mNextPC);
		JR();
	}

	void R4000::BREAK()
	{
		mCOPs[0]->signalBreak();
	}

	void R4000::SYSCALL()
	{
	}

	void R4000::CACHE()
	{
		//TODO: All caching system
	}

	void R4000::SYNC()
	{
		//NOP on R4000
	}

	void R4000::COP0()
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

	void R4000::COP1()
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

	void R4000::COP2()
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

	void R4000::COP3()
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

	void R4000::LWC0()
	{
	}

	void R4000::LWC1()
	{
	}

	void R4000::LWC2()
	{
	}

	void R4000::LWC3()
	{
	}

	void R4000::SWC0()
	{
	}

	void R4000::SWC1()
	{
	}

	void R4000::SWC2()
	{
	}

	void R4000::SWC3()
	{
	}

	void R4000::NA()
	{
	}

	void R4000::iCacheStore(U32 address, U32 value)
	{
		U32 cacheLineNumber = address / 32;
		U32 wordAddress = (address & 0xF) / 4;

		auto& instruction = mICache.CacheLines[cacheLineNumber].Words[wordAddress];
		instruction.Word = value;
		mICache.CacheLines[cacheLineNumber].Valid = ESX_FALSE;
	}

	void R4000::addWriteQueueOperation(const StoreOperation& writeOp)
	{
		mWriteQueue.push_back(writeOp);
	}

	void R4000::doWriteQueueOperation(const StoreOperation& writeOp)
	{
		switch (writeOp.Size) {
			case sizeof(U8)  :  mRootBus->store(writeOp.Address, static_cast<U8> (writeOp.Data)); break;
			case sizeof(U16) :	mRootBus->store(writeOp.Address, static_cast<U16>(writeOp.Data)); break;
			case sizeof(U32) :	mRootBus->store(writeOp.Address, static_cast<U32>(writeOp.Data)); break;
		}
	}

	BIT R4000::flushWriteQueue(U32 address)
	{
		auto foundIt = std::find_if(mWriteQueue.begin(), mWriteQueue.end(), [&](const StoreOperation& storeOp) { return storeOp.Address == address; });
		if (foundIt != mWriteQueue.end()) {
			doWriteQueueOperation(*foundIt);
			mWriteQueue.erase(foundIt);
			return ESX_TRUE;
		}
		return ESX_FALSE;
	}

	void R4000::flushWriteQueueFirst()
	{
		if (mWriteQueue.size() == 0) return;
		auto it = mWriteQueue.begin();
		doWriteQueueOperation(*it);
		mWriteQueue.erase(it);
	}

	void R4000::flushWriteQueueAll()
	{
		for (auto it = mWriteQueue.begin(); it != mWriteQueue.end(); ++it) {
			doWriteQueueOperation(*it);
		}
		mWriteQueue.clear();
	}
}
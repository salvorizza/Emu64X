#include "Core/RCP/RCP.h"
#include "R4000.h"

#include <iostream>
#include <iomanip>
#include <fstream>

#include "optick.h"

namespace esx {

	R4000::R4000(RCP* rcp)
		:	MIPSProcessor(ESX_TEXT("R4000")),
			mRCP(rcp)
	{
	}

	R4000::~R4000()
	{
	}

	void R4000::init()
	{
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
		U32 physicalAddress = 0x04001000 + (virtualAddress & 0xFFF);
		U32 output = 0;
		mRCP->load("Root", physicalAddress, output, 0, sizeof(U32) * 8);
		return output;
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

	U64 R4000::load(U32 virtualAddress, BIT& exception, size_t accessSize) {
		U32 physicalAddress = 0x04000000 + (virtualAddress & 0xFFF);

		PRINT_LOAD(physicalAddress);

		U64 result = 0;
		if (accessSize == 8) {
			U32 lo,hi;

			mRCP->load("Root", physicalAddress, hi, 0, sizeof(U32) * 8);
			mRCP->load("Root", physicalAddress + 4, lo, 0, sizeof(U32) * 8);

			result = (static_cast<U64>(hi) << 32) | lo;
		} else {
			U32 output;
			mRCP->load("Root", physicalAddress, output, 0, accessSize * 8);
			result = output;
		}

		PRINT_IO_LOAD(physicalAddress, result);

		return result;
	}

	void R4000::store(U32 virtualAddress, U64 value, size_t accessSize) {
		U32 physicalAddress = 0x04000000 + (virtualAddress & 0xFFF);

		PRINT_STORE(physicalAddress, value);
		PRINT_IO_STORE(physicalAddress, value);

		if (accessSize == 8) {
			mRCP->store("Root", physicalAddress, value >> 32, 0, sizeof(U32) * 8);
			mRCP->store("Root", physicalAddress + 4, value & 0xFFFFFFFF, 0, sizeof(U32) * 8);
		} else {
			mRCP->store("Root", physicalAddress, value, 0, accessSize * 8);
		}
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

		U64 r = load(m, exception, sizeof(U32));
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::LH()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 r = load(m, exception, sizeof(U16));
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		} else {
			r = static_cast<I16>(static_cast<U16>(r));
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::LHU()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U32 r = load(m, exception, sizeof(U16));
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::LB()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 r = load(m, exception, sizeof(U8));
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		} else {
			r = static_cast<I8>(static_cast<U8>(r));
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::LBU()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U32 r = load(m, exception, sizeof(U8));

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::SB()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 v = getRegister(mCurrentInstruction.RegisterTarget());

		U64 m = a + b;

		store(m, v, sizeof(U8));
	}

	void R4000::SH()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();
		U64 v = getRegister(mCurrentInstruction.RegisterTarget());

		U64 m = a + b;

		store(m, v, sizeof(U16));
	}

	void R4000::SC()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 v = getRegister(mCurrentInstruction.RegisterTarget());

		if (mLLBit == ESX_TRUE) {
			store(m, v, sizeof(U32));
		}

		setRegister(mCurrentInstruction.RegisterTarget(), mLLBit);
	}

	void R4000::SW()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 v = getRegister(mCurrentInstruction.RegisterTarget());

		store(m, v, sizeof(U32));
	}

	void R4000::LUI()
	{
		U32 r = static_cast<U32>(mCurrentInstruction.Immediate()) << 16;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R4000::LL()
	{
		BIT exception = ESX_FALSE;

		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = mCurrentInstruction.ImmediateSE();

		U64 m = a + b;

		U64 r = load(m, exception, sizeof(U32));
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
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

		U32 r = a << s;

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

		U32 r = a << (s & 0x1F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::SRLV()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U32 r = a >> (s & 0x1F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::SRAV()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U32 r = a >> (s & 0x1F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R4000::BEQ()
	{
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

	void R4000::BNE()
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

	void R4000::BLTZ()
	{
		mBranch = ESX_TRUE;
		I32 a = static_cast<I32>(getRegister(mCurrentInstruction.RegisterSource()));
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
		I32 a = static_cast<I32>(getRegister(mCurrentInstruction.RegisterSource()));
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
		I32 a = static_cast<I32>(getRegister(mCurrentInstruction.RegisterSource()));
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
		I32 a = static_cast<I32>(getRegister(mCurrentInstruction.RegisterSource()));
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
		I32 a = static_cast<I32>(getRegister(mCurrentInstruction.RegisterSource()));
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
		U32 a = (mNextPC & 0xF0000000) | (mCurrentInstruction.PseudoAddress() << 2);
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
		else {
			ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
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
		} else {
				ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
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
		else {
			ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
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
		else {
			ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
		}
	}

	void R4000::LWC0()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void R4000::LWC1()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void R4000::LWC2()
	{
		SharedPtr<VectorUnit> vu = std::dynamic_pointer_cast<VectorUnit>(mCOPs[2]);

		VU_LOAD_STORE_INSTRUCTION_Register instruction;
		instruction.write(mCurrentInstruction.binaryInstruction);

		RegisterIndex base = instruction.get<layouts::VU_LOAD_STORE_INSTRUCTION_Register::base>();
		U8 vt = instruction.get<layouts::VU_LOAD_STORE_INSTRUCTION_Register::vt>();
		U8 opcode = instruction.get<layouts::VU_LOAD_STORE_INSTRUCTION_Register::opcode>();
		U8 element = instruction.get<layouts::VU_LOAD_STORE_INSTRUCTION_Register::element>();
		I8 offset = instruction.get<layouts::VU_LOAD_STORE_INSTRUCTION_Register::offset>();

		switch (opcode) {
			case 0x00:
			case 0x01:
			case 0x02:
			case 0x03: {
				size_t access_size = 1llu << opcode;

				U32 addr = (getRegister(base) + offset * access_size) & 0xFFF;
				Span<U8> dmem = Span<U8>(&mRCP->mDMEM[addr], &mRCP->mDMEM[addr + access_size]);
				vu->setVPRRegisterBytes(vt, dmem, element, access_size);
				break;
			}

			case 0x04: {
				size_t access_size = 16;

				U32 addr = (getRegister(base) + offset * access_size) & 0xFFF;
				U32 end = addr | 0xF;
				U32 size = std::min<U32>(end - addr, 15 - element);

				Span<U8> dmem = Span<U8>(&mRCP->mDMEM[addr], &mRCP->mDMEM[addr + size + 1]);
				vu->setVPRRegisterBytes(vt, dmem, element, size + 1);
				break;
			}

			case 0x05: {
				size_t access_size = 16;

				U32 end = (getRegister(base) + offset * access_size) & 0xFFF;
				U32 addr = end & ~0x10;
				U32 size = std::min<U32>(end - addr, 15 - element);

				Span<U8> dmem = Span<U8>(&mRCP->mDMEM[addr], &mRCP->mDMEM[addr + size + 1]);
				vu->setVPRRegisterBytes(vt, dmem, element, size + 1);
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("Not handled LWC2 {:02x}h opcode", opcode);
				break;
			}

		}
	}

	void R4000::LWC3()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void R4000::SWC0()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void R4000::SWC1()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void R4000::SWC2()
	{
		SharedPtr<VectorUnit> vu = std::dynamic_pointer_cast<VectorUnit>(mCOPs[2]);

		VU_LOAD_STORE_INSTRUCTION_Register instruction;
		instruction.write(mCurrentInstruction.binaryInstruction);

		RegisterIndex base = instruction.get<layouts::VU_LOAD_STORE_INSTRUCTION_Register::base>();
		U8 vt = instruction.get<layouts::VU_LOAD_STORE_INSTRUCTION_Register::vt>();
		U8 opcode = instruction.get<layouts::VU_LOAD_STORE_INSTRUCTION_Register::opcode>();
		U8 element = instruction.get<layouts::VU_LOAD_STORE_INSTRUCTION_Register::element>();
		I8 offset = instruction.get<layouts::VU_LOAD_STORE_INSTRUCTION_Register::offset>();
		
		switch (opcode) {
			case 0:
			case 1:
			case 2:
			case 3: {
				size_t access_size = 1llu << opcode;

				U32 addr = (getRegister(base) + offset * access_size) & 0xFFF;
				Span<U8> data = vu->getVPRRegisterBytes(vt, element, access_size);
				memcpy(&mRCP->mDMEM[addr], data.data(), data.size_bytes());
				break;
			}

			case 0x04: {
				size_t access_size = 16;

				U32 addr = (getRegister(base) + offset * access_size) & 0xFFF;
				U32 end = addr | 0xF;
				U32 size = std::min<U32>(end - addr, 15 - element);

				Span<U8> data = vu->getVPRRegisterBytes(vt, element, size + 1);
				memcpy(&mRCP->mDMEM[addr], data.data(), data.size_bytes());
				break;
			}

			default: {
				ESX_CORE_LOG_WARNING("Not handled SWC2 {:02x}h opcode", opcode);
				break;
			}
		}
	}

	void R4000::SWC3()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}

	void R4000::NA()
	{
		ESX_CORE_LOG_WARNING("{} Not implemented yet", __FUNCTION__);
	}
}
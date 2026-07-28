#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <functional>
#include <format>
#include <queue>

#include "Base/Base.h"
#include "Base/Bus.h"
#include "../Common/Common.h"

namespace esx {

	class RCP;

	class R4000;
	typedef void(R4000::*R4000ExecuteFunction)();
	using R4000Instruction = MIPSInstruction<R4000ExecuteFunction>;

	class CPUStatusPanel;
	class DisassemblerPanel;

	using R4000iCache = Cache<256, 4>;

	class R4000 : public MIPSProcessor<U32, U32, R4000ExecuteFunction> {
	public:
		friend class CPUStatusPanel;
		friend class DisassemblerPanel;
		friend class TTYPanel;

		R4000(RCP* rcp);
		~R4000();

		void init() override;
		void clock() override;
		U32 fetch(U32 virtualAddress) override;
		void decode(R4000Instruction& result, U32 instruction, U32 address, BIT suppressException) override;
		void execute(R4000Instruction& instruction) override;
		void reset() override;

		using BusDevice::load;
		using BusDevice::store;
		U64 load(U32 virtualAddress, BIT& exception, size_t accessSize);
		void store(U32 virtualAddress, U64 value, size_t accessSize);

		//Arithmetic
		void ADD();
		void ADDU();
		void SUB();
		void SUBU();
		void ADDI();
		void ADDIU();

		//Memory
		void LW();
		void LH();
		void LHU();
		void LB();
		void LBU();
		void LUI();
		void LL();

		void SC();
		void SW();
		void SH();
		void SB();

		//Comparison
		void SLT();
		void SLTU();
		void SLTI();
		void SLTIU();

		//Binary
		void AND();
		void ANDI();
		void OR();
		void ORI();
		void XOR();
		void XORI();
		void NOR();
		void SLL();
		void SRL();
		void SRA();
		void SLLV();
		void SRLV();
		void SRAV();

		//Control
		void BEQ();
		void BNE();
		void BLTZ();
		void BLTZAL();
		void BLEZ();
		void BGTZ();
		void BGEZ();
		void BGEZAL();
		void J();
		void JR();
		void JAL();
		void JALR();
		void BREAK();
		void SYSCALL();
		void CACHE();
		void SYNC();

		//COPx
		void COP0();
		void COP1();
		void COP2();
		void COP3();
		void LWC0();
		void LWC1();
		void LWC2();
		void LWC3();
		void SWC0();
		void SWC1();
		void SWC2();
		void SWC3();

		void NA();
	private:
		RCP* mRCP;
		R4000iCache mICache = {};
		Vector<StoreOperation> mWriteQueue = {};
	};

}
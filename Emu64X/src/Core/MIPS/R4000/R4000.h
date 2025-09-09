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

		R4000();
		~R4000();

		void init() override;
		void clock() override;
		U32 fetch(U32 virtualAddress) override;
		void decode(R4000Instruction& result, U32 instruction, U32 address, BIT suppressException) override;
		void execute(R4000Instruction& instruction) override;
		void reset() override;

		template<typename T>
		U32 load(U32 virtualAddress, BIT& exception) {
			BIT cached = ESX_FALSE;
			U32 physicalAddress = 0x04000000 + (virtualAddress & 0xFFF);

			if (isWriteQueueActive(physicalAddress)) {
				if (flushWriteQueue(physicalAddress) == ESX_FALSE) {
					flushWriteQueueFirst();
				}
			}
			else {
				flushWriteQueueAll(); //TODO: Write queue stall
			}

			PRINT_LOAD(physicalAddress);

			U32 output = mRootBus->load(physicalAddress);

			PRINT_IO_LOAD(physicalAddress, output);

			return output;
		}

		template<typename T>
		void store(U32 virtualAddress, U32 value) {
			BIT cached = ESX_FALSE;
			U32 physicalAddress = 0x04000000 + (virtualAddress & 0xFFF);

			PRINT_STORE(physicalAddress, value);
			PRINT_IO_STORE(physicalAddress, value);

			if (isWriteQueueActive(physicalAddress)) {
				if (isWriteQueueFull()) {
					flushWriteQueueAll();
				}

				addWriteQueueOperation({ .Address = physicalAddress, .Data = value, .Size = sizeof(T) });
			}
			else {
				flushWriteQueueAll(); //TODO: Stall cause by write queue
				mRootBus->store(physicalAddress, value);
			}
		}

		inline BIT isWriteQueueActive(U32 address) {
			return ESX_FALSE;
		}

		inline U64 getClocks() const { return mCycles; }

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
		U32 cacheMiss(U32 virtualAddress, U32 physicalAddress, U32 cacheLineNumber, U32 tag, U32 startIndex);

		void iCacheStore(U32 address, U32 value);

		inline BIT isWriteQueueFull() { return (mWriteQueue.size() == 4) ? ESX_TRUE : ESX_FALSE; }
		void addWriteQueueOperation(const StoreOperation& writeOp);
		void doWriteQueueOperation(const StoreOperation& writeOp);
		BIT flushWriteQueue(U32 address);
		void flushWriteQueueFirst();
		void flushWriteQueueAll();
	private:
		SharedPtr<Bus> mRootBus;
		R4000iCache mICache = {};
		Vector<StoreOperation> mWriteQueue = {};
	};

}
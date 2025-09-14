#pragma once

#include <limits>

#include "Base/Base.h"
#include "Base/Bus.h"
#include "../Common/Common.h"

#include "SystemControlCoprocessor.h"
#include "Core/RCP/RCP.h"

namespace esx {

	class VR4300;
	typedef void(VR4300::* VR4300ExecuteFunction)();
	using VR4300Instruction = MIPSInstruction<VR4300ExecuteFunction>;

	class CPUStatusPanel;
	class DisassemblerPanel;

	using iCache = Cache<512, 8>;
	using dCache = Cache<512, 4>;

	class VR4300 : public MIPSProcessor<U32, U64, VR4300ExecuteFunction> {
	public:
		friend class CPUStatusPanel;
		friend class DisassemblerPanel;
		friend class TTYPanel;
		friend class Coprocessor<VR4300>;
		friend class SystemControlCoprocessor;

		VR4300();
		~VR4300();

		void clock() override;
		void init() override;
		U32 fetch(U32 virtualAddress) override;
		void decode(VR4300Instruction& result, U32 instruction, U32 address, BIT suppressException) override;
		void execute(VR4300Instruction& instruction) override;
		void reset() override;

		template<typename T>
		T load(U32 virtualAddress, BIT& exception) {
			if (ADDRESS_UNALIGNED(virtualAddress, T)) {
				raiseException(ExceptionType::AddressErrorLoad, virtualAddress);
				exception = ESX_TRUE;
				return 0;
			}

			BIT cached = ESX_FALSE;
			U32 physicalAddress = mCP0->AddressTranslation(virtualAddress, ESX_FALSE, cached);

			mCP0->watchAddress(physicalAddress, ESX_FALSE);

			if (isWriteQueueActive(physicalAddress)) {
				if (flushWriteQueue(physicalAddress) == ESX_FALSE) {
					flushWriteQueueFirst();
				}
			}
			else {
				flushWriteQueueAll(); //TODO: Write queue stall
			}

			PRINT_LOAD(physicalAddress);

			T result = 0;
			if constexpr (sizeof(T) == 8) {
				U32 lo = mRCP->SysADLoad(physicalAddress & ~0x7, sizeof(T) * 8);
				U32 hi = mRCP->SysADLoad((physicalAddress & ~0x7) + 4, sizeof(T) * 8);

				result = (static_cast<U64>(hi) << 32) | lo;
			} else {
				U32 output = mRCP->SysADLoad(physicalAddress, sizeof(T) * 8);
				constexpr T mask = std::numeric_limits<T>::max();
				result = output >> ((physicalAddress & 0x3) * 8) & mask;
			}

			PRINT_IO_LOAD(physicalAddress, result);
			
			return result;
		}

		template<typename T>
		void store(U32 virtualAddress, U64 value) {
			if (ADDRESS_UNALIGNED(virtualAddress, T)) {
				raiseException(ExceptionType::AddressErrorStore, virtualAddress);
				return;
			}

			BIT cached = ESX_FALSE;
			U32 physicalAddress = mCP0->AddressTranslation(virtualAddress, ESX_TRUE, cached);

			mCP0->watchAddress(physicalAddress, ESX_TRUE);

			PRINT_STORE(physicalAddress, value);
			PRINT_IO_STORE(physicalAddress,value);

			value <<= (physicalAddress & 0x3) * 8;
			mRCP->SysADStore(physicalAddress, sizeof(T) * 8, value);
		}

		static inline BIT isCacheActive(U32 address) {
			return (address & (1 << 29)) == 0;
		}

		inline BIT isWriteQueueActive(U32 address) {
			/*U32 sr = getCP0Register(COP0Register::SR);

			return (sr & 0x10000) == 0 && (address & (1 << 29)) == 0;*/
			return ESX_FALSE;
		}

		void raiseException(ExceptionType type, U32 virtualAddress = 0);

		inline U64 getClocks() const { return mCycles; }

		//Arithmetic
		void ADD();
		void ADDU();
		void DADD();
		void DADDU();
		void SUB();
		void SUBU();
		void DSUB();
		void DSUBU();
		void ADDI();
		void ADDIU();
		void DADDI();
		void DADDIU();
		void MULT();
		void MULTU();
		void DMULT();
		void DMULTU();
		void DIV();
		void DIVU();
		void DDIV();
		void DDIVU();
		void MFLO();
		void MTLO();
		void MFHI();
		void MTHI();

		//Memory
		void LD();
		void LDL();
		void LDR();
		void LW();
		void LWU();
		void LWL();
		void LWR();
		void LH();
		void LHU();
		void LB();
		void LBU();
		void LUI();
		void LL();
		void LLD();
		
		void SC();
		void SD();
		void SDL();
		void SDR();
		void SW();
		void SWL();
		void SWR();
		void SH();
		void SB();
		void SCD();
		

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
		void DSLL();
		void DSLL32();
		void SRL();
		void DSRL();
		void DSRL32();
		void SRA();
		void DSRA();
		void DSRA32();
		void SLLV();
		void DSLLV();
		void SRLV();
		void DSRLV();
		void SRAV();
		void DSRAV();

		//Control
		void BEQ();
		void BEQL();
		void BNE();
		void BNEL();
		void BLTZ();
		void BLTZL();
		void BLTZAL();
		void BLTZALL();
		void BLEZ();
		void BLEZL();
		void BGTZ();
		void BGTZL();
		void BGEZ();
		void BGEZL();
		void BGEZAL();
		void BGEZALL();
		void J();
		void JR();
		void JAL();
		void JALR();
		void BREAK();
		void SYSCALL();
		void CACHE();
		void SYNC();
		void TGE();
		void TGEI();
		void TGEU();
		void TGEIU();
		void TLT();
		void TLTI();
		void TLTU();
		void TLTIU();
		void TEQ();
		void TEQI();
		void TNE();
		void TNEI();

		//COPx
		void COP0();
		void COP1();
		void COP2();
		void COP3();
		void LDC1();
		void LDC2();
		void LWC0();
		void LWC1();
		void LWC2();
		void LWC3();
		void SDC1();
		void SDC2();
		void SWC0();
		void SWC1();
		void SWC2();
		void SWC3();
		

		void NA();

		BIT is64BitMode() const { return mCP0->is64BitMode(); }
		BIT isCoprocessorUsable(U8 copNumber) const { return mCP0->isCoprocessorUsable(copNumber); }
		BIT isReserved64BitInstruction() const { return mCP0->isReserved64BitInstruction(); }
	private:
		void iCacheStore(U32 address, U32 value);

		inline BIT isWriteQueueFull() { return (mWriteQueue.size() == 4) ? ESX_TRUE : ESX_FALSE; }
		void addWriteQueueOperation(const StoreOperation& writeOp);
		void doWriteQueueOperation(const StoreOperation& writeOp);
		BIT flushWriteQueue(U32 address);
		void flushWriteQueueFirst();
		void flushWriteQueueAll();
	private:
		SharedPtr<RCP> mRCP;
		SharedPtr<SystemControlCoprocessor> mCP0;
		iCache mICache = {};
		dCache mDCache = {};
		Vector<StoreOperation> mWriteQueue = {};
	};

}
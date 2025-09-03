#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <functional>
#include <format>
#include <queue>

#include "Base/Base.h"
#include "Base/Bus.h"

#include "SystemControlCoprocessor.h"

namespace esx {

	#define COP(x) (((x) >> 25) & 0x1)
	#define COP_N(x) (((x) >> 26) & 0x3)
	#define COP_FUNC(x) ((x) & 0x1F)

	constexpr U32 EXCEPTION_HANDLER_ADDRESS = 0x80000080;
	constexpr U32 BREAKPOINT_EXCEPTION_HANDLER_ADDRESS = 0x80000040;
	#define ADDRESS_UNALIGNED(x,type) (((x) & (sizeof(type) - 1)) != 0x0)
	#define OVERFLOW_ADD32(a,b,s) (~(((a) & 0x80000000) ^ ((b) & 0x80000000)) & (((a) & 0x80000000) ^ ((s) & 0x80000000)))
	#define OVERFLOW_SUB32(a,b,s) (((a) & 0x80000000) ^ ((b) & 0x80000000)) & (((a) & 0x80000000) ^ ((s) & 0x80000000))
	#define OVERFLOW_ADD64(a,b,s) (~(((a) & 0x8000000000000000) ^ ((b) & 0x8000000000000000)) & (((a) & 0x8000000000000000) ^ ((s) & 0x8000000000000000)))
	#define OVERFLOW_SUB64(a,b,s) (((a) & 0x8000000000000000) ^ ((b) & 0x8000000000000000)) & (((a) & 0x8000000000000000) ^ ((s) & 0x8000000000000000))

	#define ESX_CORE_BIOS_LOG_TRACE(x,...) //ESX_CORE_LOG_TRACE(x,__VA_ARGS__)

	#define PRINT_LOAD(x)
	#define PRINT_STORE(x,v)

	#ifdef IO_TRACE
		#define PRINT_IO_LOAD(x, o) \
		if (x >= 0x1F801000 && x < 0x1F802000) { \
			const StringView& ioName = IOMap.contains(x & ~0x1) ? IOMap.at(x & ~0x1) : IOMap.at(x & ~0x3); \
			ESX_CORE_LOG_INFO("{:08x}h - I/O Read from {}[{:08x}h] value {:08x}h", mCurrentInstruction.Address, ioName, x, o); \
		}

		#define PRINT_IO_STORE(x, v) \
		if (x >= 0x1F801000 && x < 0x1F802000) { \
			const StringView& ioName = IOMap.contains(x & ~0x1) ? IOMap.at(x & ~0x1) : IOMap.at(x & ~0x3); \
			ESX_CORE_LOG_INFO("{:08x}h - I/O Write {:08x}h to {}[{:08x}h]", mCurrentInstruction.Address, v, ioName, x); \
		}
	#else
		#define PRINT_IO_LOAD(x, o)
		#define PRINT_IO_STORE(x, v)
	#endif

	enum class GPRRegister : U8 {
		zero,
		at,
		v0, 
		v1,
		a0,a1,a2,a3,
		t0,t1,t2,t3,t4,t5,t6,t7,
		s0,s1,s2,s3,s4,s5,s6,s7,
		t8,t9,
		k0,k1,
		gp,
		sp,
		fp,
		ra
	};

	struct Instruction;
	class VR4300;
	class Coprocessor;
	typedef void(VR4300::*ExecuteFunction)();
	typedef void(SystemControlCoprocessor::*CoprocessorExecuteFunction)();
	class InterruptControl;

	struct Instruction {
		U32 Address = 0;
		U32 binaryInstruction = 0;
		ExecuteFunction Execute = nullptr;

		inline U8 Opcode() const {
			return binaryInstruction >> 26;
		}

		RegisterIndex RegisterSource() const {
			return RegisterIndex(((binaryInstruction >> 21) & 0x1F));
		}

		RegisterIndex RegisterTarget() const {
			return RegisterIndex(((binaryInstruction >> 16) & 0x1F));
		}

		RegisterIndex RegisterDestination() const  {
			return RegisterIndex(((binaryInstruction >> 11) & 0x1F));
		}

		U8 ShiftAmount() const  {
			return ((binaryInstruction >> 6) & 0x1F);
		}

		U8 Function() const {
			return (binaryInstruction & 0x3F);
		}

		U16 Immediate() const {
			return (binaryInstruction & 0xFFFF);
		}

		I32 ImmediateSE() const {
			return static_cast<I32>(static_cast<I16>(Immediate()));
		}

		U32 Code() const {
			return ((binaryInstruction >> 6) & 0xFFFFF);
		}

		U32 PseudoAddress() const {
			return (binaryInstruction & 0x3FFFFFF);
		}

		U32 Immediate25() const {
			return PseudoAddress();
		}

		U8 CoprocessorFunction() const {
			return (binaryInstruction & 0x1F);
		}

		String Mnemonic(const SharedPtr<VR4300>& cpuState) const;
	};

	class CPUStatusPanel;
	class DisassemblerPanel;

	struct InstructionCache {
		U32 Word = 0;
	};

	template<size_t W>
	struct CacheLine {
		U32 Tag = 0;
		BIT Valid = ESX_FALSE;
		BIT Dirty = ESX_FALSE;
		Array<InstructionCache, W> Instructions = {};
	};

	template<size_t L, size_t W>
	struct Cache {
		Array<CacheLine<W>, L> CacheLines = {};
	};

	using iCache = Cache<512, 8>;
	using dCache = Cache<512, 4>;

	struct StoreOperation {
		U32 Address = 0;
		U32 Data = 0;
		U32 Size = 0;
	};

	class VR4300 : public BusDevice {
	public:
		friend class CPUStatusPanel;
		friend class DisassemblerPanel;
		friend class TTYPanel;
		friend class Coprocessor;
		friend class SystemControlCoprocessor;

		VR4300();
		~VR4300();

		virtual void init() override;
		void clock();
		U32 fetch(U32 virtualAddress);
		void decode(Instruction& result, U32 instruction, U32 address, BIT suppressException = ESX_FALSE);
		virtual void reset();

		template<typename T>
		U32 load(U32 virtualAddress, BIT& exception) {
			if (ADDRESS_UNALIGNED(virtualAddress,T)) {
				raiseException(ExceptionType::AddressErrorLoad, virtualAddress);
				exception = ESX_TRUE;
				return 0;
			}

			BIT cached = ESX_FALSE;
			U32 physicalAddress = mCP0.AddressTranslation(virtualAddress, ESX_FALSE, cached);

			mCP0.watchAddress(physicalAddress, ESX_FALSE);

			if (isWriteQueueActive(physicalAddress)) {
				if (flushWriteQueue(physicalAddress) == ESX_FALSE) {
					flushWriteQueueFirst();
				}
			} else {
				flushWriteQueueAll(); //TODO: Write queue stall
			}

			PRINT_LOAD(physicalAddress);

			T output = mRootBus->load<T>(physicalAddress);

			PRINT_IO_LOAD(physicalAddress, output);
			
			return output;
		}

		template<typename T>
		void store(U32 virtualAddress, U32 value) {
			if (ADDRESS_UNALIGNED(virtualAddress, T)) {
				raiseException(ExceptionType::AddressErrorStore, virtualAddress);
				return;
			}

			BIT cached = ESX_FALSE;
			U32 physicalAddress = mCP0.AddressTranslation(virtualAddress, ESX_TRUE, cached);

			mCP0.watchAddress(physicalAddress, ESX_TRUE);

			PRINT_STORE(physicalAddress, value);
			PRINT_IO_STORE(physicalAddress,value);

			if (isWriteQueueActive(physicalAddress)) {
				if (isWriteQueueFull()) {
					flushWriteQueueAll();
				}

				addWriteQueueOperation({ .Address = physicalAddress, .Data = value, .Size = sizeof(T) });
			} else {
				flushWriteQueueAll(); //TODO: Stall cause by write queue
				mRootBus->store<T>(physicalAddress, value);
			}
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

		Instruction mCurrentInstruction;

		U64 getRegister(RegisterIndex index);
	private:
		inline void addPendingLoad(RegisterIndex index, U64 value);
		inline void resetPendingLoad();

		void setRegister(RegisterIndex index, U64 value);

		void setCP0Register(RegisterIndex index, U32 value);
		U32 getCP0Register(RegisterIndex index);

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

		Array<U64, 32> mRegisters;

		SystemControlCoprocessor mCP0;

		Pair<RegisterIndex, U64> mPendingLoad;
		Pair<RegisterIndex, U64> mMemoryLoad;
		Pair<RegisterIndex, U64> mWriteBack;

		U64 mPC = 0;
		U64 mNextPC = 0;
		U64 mCurrentPC = 0;
		U64 mCallPC = 0;
		U64 mHI = 0;
		U64 mLO = 0;
		BIT mLLBit = ESX_FALSE;

		iCache mICache = {};
		dCache mDCache = {};

		Vector<StoreOperation> mWriteQueue = {};
		BIT mStall = ESX_FALSE;

		BIT mBranch = ESX_FALSE;
		BIT mBranchSlot = ESX_FALSE;
		BIT mNullifyBranchSlot = ESX_FALSE;
		BIT mTookBranch = ESX_FALSE;
		BIT mTookBranchSlot = ESX_FALSE;

		float mGPUClock = 0;

		U64 mCycles = 0;
		U64 mCyclesToWait = 0;

		SharedPtr<InterruptControl> mInterruptControl;
	};

}
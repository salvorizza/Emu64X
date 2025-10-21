#pragma once

#include "Base/Bus.h"
#include "Base/Base.h"

#include "Coprocessor.h"

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
		a0, a1, a2, a3,
		t0, t1, t2, t3, t4, t5, t6, t7,
		s0, s1, s2, s3, s4, s5, s6, s7,
		t8, t9,
		k0, k1,
		gp,
		sp,
		fp,
		ra
	};

	template<typename ExecuteFunction>
	struct MIPSInstruction {
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

		RegisterIndex RegisterDestination() const {
			return RegisterIndex(((binaryInstruction >> 11) & 0x1F));
		}

		U8 Element() const {
			return ((binaryInstruction >> 7) & 0xF);
		}

		U8 ShiftAmount() const {
			return ((binaryInstruction >> 6) & 0x1F);
		}

		U8 Function() const {
			return (binaryInstruction & 0x3F);
		}

		I8 Offset() const {
			return static_cast<I8>((binaryInstruction & 0x7F) << 1) >> 1;
		}


		U8 Cond() const {
			return (binaryInstruction & 0xF);
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

		template<typename T>
		String Mnemonic(const SharedPtr<T>& cpuState) const;
	};

	struct CacheWord {
		U32 Word = 0;
	};

	template<size_t W>
	struct CacheLine {
		U32 Tag = 0;
		BIT Valid = ESX_FALSE;
		BIT Dirty = ESX_FALSE;
		Array<CacheWord, W> Words = {};
	};

	template<size_t L, size_t W>
	struct Cache {
		Array<CacheLine<W>, L> CacheLines = {};

		constexpr size_t CacheLineSize() const { return L; }
		constexpr size_t NumWords() const { return W; }
	};

	struct StoreOperation {
		U32 Address = 0;
		U32 Data = 0;
		U32 Size = 0;
	};

	template<typename Address, typename Register, typename Execute>
	class MIPSProcessor : public BusDevice {
	public:
		MIPSProcessor(const StringView& name) : BusDevice(name) { reset(); }
		virtual ~MIPSProcessor() = default;

		virtual void clock() {
			if (mHalt == ESX_FALSE) {
				if (mCyclesToWait == 0) {
					if (!mStall) {
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

						if ((mBranchSlot == ESX_TRUE && mNullifyBranchSlot == ESX_FALSE) || mBranchSlot == ESX_FALSE) {
							if (opcode != 0 && mCurrentInstruction.Execute) {
								execute(mCurrentInstruction);
							}
						}
						else {
							mNullifyBranchSlot = ESX_FALSE;
						}

						mRegisters[mMemoryLoad.first] = mMemoryLoad.second;
						mRegisters[0] = 0;
						mMemoryLoad = mPendingLoad;
						resetPendingLoad();

						mRegisters[mWriteBack.first] = mWriteBack.second;
						mWriteBack.first = RegisterIndex(0);
						mRegisters[0] = 0;
					}
					mCyclesToWait = 1;
				}
				mCyclesToWait--;
			}
			mCycles++;
		}

		virtual U32 fetch(Address virtualAddress) = 0;
		virtual void decode(MIPSInstruction<Execute>& result, U32 instruction, Address address, BIT suppressException = ESX_FALSE) = 0;
		virtual void execute(MIPSInstruction<Execute>& instruction) = 0;
		virtual void reset() override {
			mRegisters = {};

			mMemoryLoad = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
			mPendingLoad = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
			mWriteBack = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);

			mPC = 0;
			mNextPC = 0;
			mCurrentPC = 0;
			mCallPC = 0;
			mHI = 0;
			mLO = 0;
			mLLBit = ESX_FALSE;

			mStall = ESX_FALSE;

			mBranch = ESX_FALSE;
			mBranchSlot = ESX_FALSE;
			mTookBranch = ESX_FALSE;
			mTookBranchSlot = ESX_FALSE;

			mCycles = 0;

			mMemoryLoad = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
			mPendingLoad = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
			mWriteBack = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
		}

		void setHalt(BIT value) { mHalt = value; }
		BIT getHalt() const { return mHalt; }

		inline U64 getClocks() const { return mCycles; }

		Register getRegister(RegisterIndex index) { return mRegisters[index.Value]; }

		virtual BIT is64BitMode() const { return ESX_FALSE; }
		virtual BIT isCoprocessorUsable(U8 copNumber) const { return ESX_TRUE; }
		virtual BIT isReserved64BitInstruction() const { return ESX_FALSE; }

		inline void addPendingLoad(RegisterIndex index, Register value)
		{
			mPendingLoad.first = index;
			mPendingLoad.second = value;

			if (mMemoryLoad.first == index) {
				mMemoryLoad = std::make_pair<RegisterIndex, U64>(RegisterIndex(0), 0);
			}
		}

		inline void resetPendingLoad()
		{
			mPendingLoad.first = RegisterIndex(0);
			mPendingLoad.second = 0;
		}

		inline void setRegister(RegisterIndex index, Register value)
		{
			mWriteBack.first = index;
			mWriteBack.second = value;
		}

		template<typename T, typename... ARGS>
		SharedPtr<T> registerCoprocessor(U8 number, ARGS&&... args) {
			auto cop = MakeShared<T>(std::forward<ARGS>(args)...);
			mCOPs[number] = cop;
			return cop;
		}
	protected:
		template<typename T>
		U32 accessCache(T& cache, U32 virtualAddress, U32 physicalAddress)
		{
			U32 index = (virtualAddress >> 2) & (cache.NumWords() - 1);
			U32 cacheLineNumber = (virtualAddress >> (2 + std::popcount(cache.NumWords() - 1))) & (cache.CacheLineSize() - 1);
			U32 tag = physicalAddress >> 12;

			auto& cacheLine = cache.CacheLines[cacheLineNumber];
			if (cacheLine.Tag == tag && cacheLine.Valid) {
				return cacheLine.Words[index].Word;
			}
			else {
				return cacheMiss(cache, virtualAddress, physicalAddress, cacheLineNumber, tag, index);
			}
		}

		template<typename T>
		U32 cacheMiss(T& cache, U32 virtualAddress, U32 physicalAddress, U32 cacheLineNumber, U32 tag, U32 startIndex)
		{
			auto& cacheLine = cache.CacheLines[cacheLineNumber];

			U32 baseAddr = physicalAddress & ~((1 << (std::popcount(cache.NumWords() - 1) + 2)) - 1);
			for (U32 index = 0; index < cacheLine.Words.size(); index++) {
				auto& word = cacheLine.Words[index];
				word.Word = mRootBus->load(baseAddr + index * sizeof(U32), 0, sizeof(U32) * 8);
			}

			cacheLine.Tag = tag;
			cacheLine.Valid = ESX_TRUE;

			return cacheLine.Words[startIndex].Word;
		}
	public:
		MIPSInstruction<Execute> mCurrentInstruction;
		Array<SharedPtr<ICoprocessor>, 4> mCOPs;
		BIT mStall = ESX_FALSE;
		BIT mBranch = ESX_FALSE;
		BIT mBranchSlot = ESX_FALSE;
		BIT mNullifyBranchSlot = ESX_FALSE;
		BIT mTookBranch = ESX_FALSE;
		BIT mTookBranchSlot = ESX_FALSE;
		BIT mHalt = ESX_FALSE;
		Register mNextPC = 0;
		Register mPC = 0;
		Register mCurrentPC = 0;
	protected:
		SharedPtr<Bus> mRootBus;
		Array<Register, 32> mRegisters;

		Pair<RegisterIndex, Register> mPendingLoad;
		Pair<RegisterIndex, Register> mMemoryLoad;
		Pair<RegisterIndex, Register> mWriteBack;

		Register mCallPC = 0;
		Register mHI = 0;
		Register mLO = 0;
		BIT mLLBit = ESX_FALSE;

		

		U64 mCycles = 0;
		U64 mCyclesToWait = 0;
	};

	template<typename ExecuteFunction>
	template<typename T>
	inline String MIPSInstruction<ExecuteFunction>::Mnemonic(const SharedPtr<T>& cpuState) const
	{
		constexpr static std::array<StringView, 32> registersMnemonics = {
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

		constexpr static std::array<StringView, 32> fpuFmts = {
			ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),
			ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),
			ESX_TEXT("s"),ESX_TEXT("d"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("w"),ESX_TEXT("l"),ESX_TEXT("unk"),ESX_TEXT("unk"),
			ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk"),ESX_TEXT("unk")
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
					case 0x0F: {
						return FormatString(ESX_TEXT("sync"));
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
					case 0x14: {
						return FormatString(ESX_TEXT("dsllv {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x16: {
						return FormatString(ESX_TEXT("dsrlv {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x17: {
						return FormatString(ESX_TEXT("dsrav {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()]);
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
					case 0x1C: {
						return FormatString(ESX_TEXT("dmult {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x1D: {
						return FormatString(ESX_TEXT("dmultu {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x1E: {
						return FormatString(ESX_TEXT("ddiv {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x1F: {
						return FormatString(ESX_TEXT("ddivu {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
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
					case 0x2C: {
						if (RegisterTarget().Value == 0) {
							return FormatString(ESX_TEXT("dmove {},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()]);
						}
						else {
							return FormatString(ESX_TEXT("dadd {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
						}
					}
					case 0x2D: {
						if (RegisterTarget().Value == 0) {
							return FormatString(ESX_TEXT("dmove {},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()]);
						}
						else {
							return FormatString(ESX_TEXT("daddu {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
						}
					}
					case 0x2E: {
						return FormatString(ESX_TEXT("dsub {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x2F: {
						return FormatString(ESX_TEXT("dsubu {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x30: {
						return FormatString(ESX_TEXT("tge {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x31: {
						return FormatString(ESX_TEXT("tgeu {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x32: {
						return FormatString(ESX_TEXT("tlt {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x33: {
						return FormatString(ESX_TEXT("tltu {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x34: {
						return FormatString(ESX_TEXT("teq {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x36: {
						return FormatString(ESX_TEXT("tne {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x38: {
						return FormatString(ESX_TEXT("dsll {},{},0x{:02x}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], ShiftAmount());
					}
					case 0x3A: {
						return FormatString(ESX_TEXT("dsrl {},{},0x{:02x}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], ShiftAmount());
					}
					case 0x3B: {
						return FormatString(ESX_TEXT("dsra {},{},0x{:02x}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], ShiftAmount());
					}
					case 0x3C: {
						return FormatString(ESX_TEXT("dsll32 {},{},0x{:02x}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], ShiftAmount());
					}
					case 0x3E: {
						return FormatString(ESX_TEXT("dsrl32 {},{},0x{:02x}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], ShiftAmount());
					}
					case 0x3F: {
						return FormatString(ESX_TEXT("dsra32 {},{},0x{:02x}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], ShiftAmount());
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
							case 0x02: {
								return FormatString(ESX_TEXT("bltzl {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
							}
							case 0x03: {
								return FormatString(ESX_TEXT("bgezl {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
							}
							case 0x08: {
								return FormatString(ESX_TEXT("tgei {},,0x{:04x}"), registersMnemonics[(U8)RegisterSource()], (I16)Immediate());
							}
							case 0x09: {
								return FormatString(ESX_TEXT("tgeiu {},,0x{:04x}"), registersMnemonics[(U8)RegisterSource()], Immediate());
							}
							case 0x0A: {
								return FormatString(ESX_TEXT("tlti {},,0x{:04x}"), registersMnemonics[(U8)RegisterSource()], (I16)Immediate());
							}
							case 0x0B: {
								return FormatString(ESX_TEXT("tltiu {},,0x{:04x}"), registersMnemonics[(U8)RegisterSource()], Immediate());
							}
							case 0x0C: {
								return FormatString(ESX_TEXT("teqi {},,0x{:04x}"), registersMnemonics[(U8)RegisterSource()], (I16)Immediate());
							}
							case 0x0E: {
								return FormatString(ESX_TEXT("tnei {},,0x{:04x}"), registersMnemonics[(U8)RegisterSource()], (I16)Immediate());
							}
							case 0x10: {
								return FormatString(ESX_TEXT("bltzal {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
							}
							case 0x11: {
								return FormatString(ESX_TEXT("bgezal {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
							}
							case 0x12: {
								return FormatString(ESX_TEXT("bltzall {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
							}
							case 0x13: {
								return FormatString(ESX_TEXT("bgezall {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
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
						U8 cpn = COP_N(binaryInstruction);
						if (COP(binaryInstruction) == 0) {
							switch (RegisterSource().Value) {
								case 0x00: {
									return FormatString(ESX_TEXT("mfc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x01: {
									return FormatString(ESX_TEXT("dmfc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x02: {
									return FormatString(ESX_TEXT("cfc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x04: {
									return FormatString(ESX_TEXT("mtc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x05: {
									return FormatString(ESX_TEXT("dmtc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x06: {
									return FormatString(ESX_TEXT("ctc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x08: {
									switch (RegisterTarget()) {
										case 0: return FormatString(ESX_TEXT("bc{}f 0x{:04x}"), cpn, Immediate());
										case 1: return FormatString(ESX_TEXT("bc{}t 0x{:04x}"), cpn, Immediate());
										case 2: return FormatString(ESX_TEXT("bc{}fl 0x{:04x}"), cpn, Immediate());
										case 3: return FormatString(ESX_TEXT("bc{}tl 0x{:04x}"), cpn, Immediate());
									}
								}
							}
						}
						else {
							switch (cpn) {
								case 0: {
									switch (CoprocessorFunction()) {
										case 1: {
											return FormatString(ESX_TEXT("tlbr"));
										}

										case 2: {
											return FormatString(ESX_TEXT("tlbwi"));
										}

										case 6: {
											return FormatString(ESX_TEXT("tlbwr"));
										}

										case 8: {
											return FormatString(ESX_TEXT("tlbp"));
										}

										case 16: {
											break;
										}

										case 24: {
											return FormatString(ESX_TEXT("eret"));
										}
									}
									break;
								}

								case 1: {
									switch (Function()) {
										case 0: {
											return FormatString(ESX_TEXT("add.{} ${},${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value, RegisterTarget().Value);
										}
										case 1: {
											return FormatString(ESX_TEXT("sub.{} ${},${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value, RegisterTarget().Value);
										}
										case 2: {
											return FormatString(ESX_TEXT("mul.{} ${},${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value, RegisterTarget().Value);
										}
										case 3: {
											return FormatString(ESX_TEXT("div.{} ${},${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value, RegisterTarget().Value);
										}
										case 4: {
											return FormatString(ESX_TEXT("sqrt.{} ${},${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value, RegisterTarget().Value);
										}
										case 5: {
											return FormatString(ESX_TEXT("abs.{} ${},${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value, RegisterTarget().Value);
										}
										case 6: {
											return FormatString(ESX_TEXT("mov.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
										}
										case 7: {
											return FormatString(ESX_TEXT("neg.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
										}
										case 8: {
											return FormatString(ESX_TEXT("round.l.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
										}
										case 9: {
											return FormatString(ESX_TEXT("trunc.l.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
										}
										case 10: {
											return FormatString(ESX_TEXT("ceil.l.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
										}
										case 11: {
											return FormatString(ESX_TEXT("floor.l.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
										}
										case 12: {
											return FormatString(ESX_TEXT("round.w.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
										}
										case 13: {
											return FormatString(ESX_TEXT("trunc.w.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
										}
										case 14: {
											return FormatString(ESX_TEXT("ceil.w.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
										}
										case 15: {
											return FormatString(ESX_TEXT("floor.w.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
										}
										case 32: {
											return FormatString(ESX_TEXT("cvt.s.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
										}
										case 33: {
											return FormatString(ESX_TEXT("cvt.d.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
										}
										case 36: {
											return FormatString(ESX_TEXT("cvt.w.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
										}
										case 37: {
											return FormatString(ESX_TEXT("cvt.l.{} ${},${}"), fpuFmts[RegisterSource().Value], ShiftAmount(), RegisterDestination().Value);
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
											return FormatString(ESX_TEXT("c.cond.{} ${},${}"), fpuFmts[RegisterSource().Value], RegisterDestination().Value, RegisterTarget().Value);
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

					case 0x14: {
						return FormatString(ESX_TEXT("beql {},{},0x{:08x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
					}
					case 0x15: {
						return FormatString(ESX_TEXT("bnel {},{},0x{:08x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
					}
					case 0x16: {
						return FormatString(ESX_TEXT("blezl {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
					}
					case 0x17: {
						return FormatString(ESX_TEXT("bgtzl {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
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
					case 0x27: {
						return FormatString(ESX_TEXT("lwu {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
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
					case 0x2C: {
						return FormatString(ESX_TEXT("sdl {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x2D: {
						return FormatString(ESX_TEXT("sdr {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x2E: {
						return FormatString(ESX_TEXT("swr {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x2F: {
						return FormatString(ESX_TEXT("cache"));
					}


					case 0x30:
					case 0x31:
					case 0x32: {
						U8 cpn = COP_N(binaryInstruction);
						return FormatString(ESX_TEXT("lwc{} ${},0x{:08x}"), cpn, (U8)RegisterTarget(), cpuState->getRegister(RegisterSource()) + ImmediateSE());
						break;
					}

					case 0x34:
					case 0x35:
					case 0x36: {
						U8 cpn = COP_N(binaryInstruction);
						return FormatString(ESX_TEXT("ldc{} ${},0x{:08x}"), cpn, (U8)RegisterTarget(), cpuState->getRegister(RegisterSource()) + ImmediateSE());
						break;
					}

					case 0x37: {
						return FormatString(ESX_TEXT("ld {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}

					case 0x38:
					case 0x39:
					case 0x3A: {
						U8 cpn = COP_N(binaryInstruction);
						return FormatString(ESX_TEXT("swc{} ${},0x{:08x}"), cpn, (U8)RegisterTarget(), cpuState->getRegister(RegisterSource()) + ImmediateSE());
						break;
					}

					case 0x3C:
					case 0x3D:
					case 0x3E: {
						U8 cpn = COP_N(binaryInstruction);
						return FormatString(ESX_TEXT("sdc{} ${},0x{:08x}"), cpn, (U8)RegisterTarget(), cpuState->getRegister(RegisterSource()) + ImmediateSE());
						break;
					}

					case 0x3F: {
						return FormatString(ESX_TEXT("sd {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
				}
			}
		}

		return FormatString(ESX_TEXT("0x{:08x}"), binaryInstruction);
	}
}
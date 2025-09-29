#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <functional>
#include <format>
#include <queue>

#define INDEX_FIELDS(M)  M(Index, 0, 5)	M(Probe, 31, 31)
#define RANDOM_FIELDS(M) M(Random,0,5)
#define ENTRYLO_FIELDS(M) M(G,0,0) M(V,1,1) M(D,2,2) M(C,3,5) M(PFN,6,25)
#define ENTRYHI_FIELDS(M) M(ASID,0,7) M(G,12,13) M(VPN2,13,31)
#define PAGEMASK_FIELDS(M) M(MASK,13,24)
#define WIRED_FIELDS(M) M(Wired,0,5)
#define PRID_FIELDS(M) M(Rev,0,7) M(Imp,8,15)
#define CONFIG_FIELDS(M) M(K0,0,2) M(CU,3,4) M(BE,15,15) M(EP,24,27) M(EC,28,30)
#define LLADDR_FIELDS(M) M(Value,0,31)
#define TAGLO_FIELDS(M) M(PState,6,7) M(PTagLo,8,27)
#define TAGHI_FIELDS(M) M(Value,0,31)
#define CONTEXT_FIELDS(M) M(BadVPN2,4,22) M(PTEBase,23,63)
#define BADVADDR_FIELDS(M) M(Value,0,63)
#define COUNT_FIELDS(M) M(Value,0,31)
#define COMPARE_FIELDS(M) M(Value,0,31)
#define STATUS_FIELDS(M) \
    M(IE,0,0) M(EXL,1,1) M(ERL,2,2) M(KSU,3,4) M(UX,5,5) M(SX,6,6) M(KX,7,7) \
    M(IM,8,15) M(DE,16,16) M(CR,17,17) M(CH,18,18) M(SR,20,20) M(TS,21,21) \
    M(BEV,22,22) M(ITS,24,24) M(RE,25,25) M(FR,26,26) M(RP,27,27) M(CU,28,31)
#define CAUSE_FIELDS(M) M(ExcCode,2,6) M(IP,8,15) M(CE,28,29) M(BD,31,31)
#define EPC_FIELDS(M) M(Value,0,31)
#define WATCHLO_FIELDS(M) M(W,0,0) M(R,1,1) M(PAddr0,3,31)
#define WATCHHI_FIELDS(M) M(PAddr1,0,3)
#define XCONTEXT_FIELDS(M) M(BadVPN2,4,30) M(R,31,32) M(PTEBase,33,63)
#define PERR_FIELDS(M) M(Diagnostic,0,7)
#define CACHEERR_FIELDS(M) M(Value,0,31)
#define ERROREPC_FIELDS(M) M(Value,0,63)

#include "Base/Base.h"
#include "Base/Bus.h"

#include "../Common/Coprocessor.h"

namespace esx {

	class VR4300;



	DEFINE_REGISTER_LAYOUT(IndexRegister, U32, INDEX_FIELDS)
	DEFINE_REGISTER_LAYOUT(RandomRegister, U32, RANDOM_FIELDS)
	DEFINE_REGISTER_LAYOUT(EntryLoRegister, U32, ENTRYLO_FIELDS)
	DEFINE_REGISTER_LAYOUT(EntryHiRegister, U32, ENTRYHI_FIELDS)
	DEFINE_REGISTER_LAYOUT(PageMaskRegister, U32, PAGEMASK_FIELDS)
	DEFINE_REGISTER_LAYOUT(WiredRegister, U32, WIRED_FIELDS)
	DEFINE_REGISTER_LAYOUT(PRIdRegister, U32, PRID_FIELDS)
	DEFINE_REGISTER_LAYOUT(ConfigRegister, U32, CONFIG_FIELDS)
	DEFINE_REGISTER_LAYOUT(LLAddrRegister, U32, LLADDR_FIELDS)
	DEFINE_REGISTER_LAYOUT(TagLoRegister, U32, TAGLO_FIELDS)
	DEFINE_REGISTER_LAYOUT(TagHiRegister, U32, TAGHI_FIELDS)
	DEFINE_REGISTER_LAYOUT(ContextRegister, U64, CONTEXT_FIELDS)
	DEFINE_REGISTER_LAYOUT(BadVAddrRegister, U64, BADVADDR_FIELDS)
	DEFINE_REGISTER_LAYOUT(CountRegister, U32, COUNT_FIELDS)
	DEFINE_REGISTER_LAYOUT(CompareRegister, U32, COMPARE_FIELDS)
	DEFINE_REGISTER_LAYOUT(StatusRegister, U32, STATUS_FIELDS)
	DEFINE_REGISTER_LAYOUT(CauseRegister, U32, CAUSE_FIELDS)
	DEFINE_REGISTER_LAYOUT(EPCRegister, U32, EPC_FIELDS)
	DEFINE_REGISTER_LAYOUT(WatchLoRegister, U32, WATCHLO_FIELDS)
	DEFINE_REGISTER_LAYOUT(WatchHiRegister, U32, WATCHHI_FIELDS)
	DEFINE_REGISTER_LAYOUT(XContextRegister, U64, XCONTEXT_FIELDS)
	DEFINE_REGISTER_LAYOUT(PErrRegister, U32, PERR_FIELDS)
	DEFINE_REGISTER_LAYOUT(CacheErrRegister, U32, CACHEERR_FIELDS)
	DEFINE_REGISTER_LAYOUT(ErrorEPCRegister, U64, ERROREPC_FIELDS)

	enum class SystemControlRegisterType : U8 {
		Index = 0,
		Random = 1,
		EntryLo0 = 2,
		EntryLo1 = 3,
		Context = 4,
		PageMask = 5,
		Wired = 6,
		BadVAddr = 8,
		Count = 9,
		EntryHi = 10,
		Compare = 11,
		Status = 12,
		Cause = 13,
		EPC = 14,
		PRId = 15,
		Config = 16,
		LLAddr = 17,
		WatchLo = 18,
		WatchHi = 19,
		XContext = 20,
		PErr = 26,
		CacheErr = 27,
		TagLo = 28,
		TagHi = 29,
		ErrorEPC = 30
	};

	enum class ExceptionType : U8 {
		Interrupt = 0,
		TLBMod = 1,
		TLBMissLoadFetch = 2,
		TLBMissStore = 3,
		AddressErrorLoad = 4,
		AddressErrorStore = 5,
		InstructionBusError = 6,
		DataBusError = 7,
		Syscall = 8,
		Breakpoint = 9,
		ReservedInstruction = 10,
		CoprocessorUnusable = 11,
		ArithmeticOverflow = 12,
		Trap = 13,
		FloatingPoint = 15,
		Watch = 23
	};

	enum class TLBExceptionType : U8 {
		TLBMod = 1,
		TLBMissLoadFetch = 2,
		TLBMissStore = 3,
		TLBInvalidLoadFetch = 4,
		TLBInvalidStore = 5
	};

	enum class OperatingMode : U8 {
		Kernel = 0b00,
		Supervisor = 0b01,
		User = 0b10
	};

	enum class Interrupt : U8 {
		IP0 = 1 << 0,
		IP1 = 1 << 1,
		IP2 = 1 << 2,
		IP3 = 1 << 3,
		IP4 = 1 << 4,
		IP5 = 1 << 5,
		IP6 = 1 << 6,
		IP7 = 1 << 7
	};

	struct TLBEntry {
		EntryLoRegister	   EntryLo0;
		EntryLoRegister    EntryLo1;
		EntryHiRegister    EntryHi;
		PageMaskRegister   PageMask;
	};
	using TLB = Array<TLBEntry, 32>;

	class SystemControlCoprocessor : public Coprocessor<VR4300> {
		friend class CPUStatusPanel;
	public:
		SystemControlCoprocessor(VR4300* cpu);
		~SystemControlCoprocessor() = default;

		void clock(U64 clocks) override;

		void CO() override;

		void unusable() override;
		void reserved() override;

		void TLBR();
		void TLBWI();
		void TLBWR();
		void TLBP();
		void ERET();

		void handleInterrupts();
		void raiseException(ExceptionType type, U32 parameter = 0);
		void raiseTLBException(TLBExceptionType type, U32 virtualAddress);

		void watchAddress(U32 physicalAddress, BIT store);
		U32 AddressTranslation(U32 virtualAddress, BIT store, BIT& cached);

		virtual U64 getRegister(RegisterIndex reg) override;
		virtual void setRegister(RegisterIndex reg, U64 value) override;

		void clearInterrupt(Interrupt interrupt);
		void generateInterrupt(Interrupt interrupt);
		OperatingMode getCurrentOperatingMode() const;
		BIT isAddressLegal(U32 virtualAddress) const;
		BIT isAdressMapped(U32 virtualAddress) const;
		BIT is64BitMode() const;
		BIT isCoprocessorUsable(U8 copNumber) const;
		BIT isReserved64BitInstruction() const;
		BIT areInterruptsPending() const;
		BIT areInterruptsEnabled() const;
		void setLLAddrToLastTranslation();
		BIT useAdditionalFPR();
	private:
		IndexRegister      mIndexRegister;
		RandomRegister     mRandomRegister;
		EntryLoRegister	   mEntryLo0Register;
		EntryLoRegister    mEntryLo1Register;
		EntryHiRegister    mEntryHiRegister;
		PageMaskRegister   mPageMaskRegister;
		WiredRegister      mWiredRegister;
		PRIdRegister       mPRIdRegister;
		ConfigRegister     mConfigRegister;
		LLAddrRegister     mLLAddrRegister;
		TagLoRegister      mTagLoRegister;
		TagHiRegister      mTagHiRegister;
		ContextRegister    mContextRegister;
		BadVAddrRegister   mBadVAddrRegister;
		CountRegister      mCountRegister;
		CompareRegister    mCompareRegister;
		StatusRegister     mStatusRegister;
		CauseRegister      mCauseRegister;
		EPCRegister        mEPCRegister;
		WatchLoRegister    mWatchLoRegister;
		WatchHiRegister    mWatchHiRegister;
		XContextRegister   mXContextRegister;
		PErrRegister       mPErrRegister;
		CacheErrRegister   mCacheErrRegister;
		ErrorEPCRegister   mErrorEPCRegister;

		TLB mTLB;

		U32 mLastPhysicalAddress;
	};
}
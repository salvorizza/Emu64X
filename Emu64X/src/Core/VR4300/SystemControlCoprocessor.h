#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <functional>
#include <format>
#include <queue>

#include "Base/Base.h"
#include "Base/Bus.h"

#include "Coprocessor.h"

namespace esx {

	class VR4300;

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

	struct IndexRegisterLayout {
		enum class Field { Index, Probe };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
				case Field::Index: return { 0, 5 };
				case Field::Probe: return { 30, 31 };
			}
		}
	};
	using IndexRegister = Register<IndexRegisterLayout, U32>;

	struct RandomRegisterLayout {
		enum class Field { Random };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
				case Field::Random: return { 0, 5 };
			}
		}
	};
	using RandomRegister = Register<RandomRegisterLayout, U32>;

	struct EntryLo0RegisterLayout {
		enum class Field { Value };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::Value: return { 0, 31 };
			}
		}
	};
	using EntryLo0Register = Register<EntryLo0RegisterLayout, U32>;

	struct EntryLo1RegisterLayout {
		enum class Field { Value };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::Value: return { 0, 31 };
			}
		}
	};
	using EntryLo1Register = Register<EntryLo1RegisterLayout, U32>;

	struct EntryHiRegisterLayout {
		enum class Field { Value };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::Value: return { 0, 31 };
			}
		}
	};
	using EntryHiRegister = Register<EntryHiRegisterLayout, U32>;

	struct PageMaskRegisterLayout {
		enum class Field { Value };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::Value: return { 0, 31 };
			}
		}
	};
	using PageMaskRegister = Register<PageMaskRegisterLayout, U32>;

	struct WiredRegisterLayout {
		enum class Field { Wired };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
				case Field::Wired: return { 0, 5 };
			}
		}
	};
	using WiredRegister = Register<WiredRegisterLayout, U32>;

	struct PRIdRegisterLayout {
		enum class Field { Rev, Imp };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
				case Field::Rev: return { 0, 7 };
				case Field::Imp: return { 8, 15 };
			}
		}
	};
	using PRIdRegister = Register<PRIdRegisterLayout, U32>;

	struct ConfigRegisterLayout {
		enum class Field { K0,CU,BE,EP,EC };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::K0: return { 0, 2 };
			case Field::CU: return { 3, 4 };
			case Field::BE: return { 14, 15 };
			case Field::EP: return { 24, 27 };
			case Field::EC: return { 28, 30 };
			}
		}
	};
	using ConfigRegister = Register<ConfigRegisterLayout, U32>;
	
	struct LLAddrRegisterLayout {
		enum class Field { Value };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::Value: return { 0, 31 };
			}
		}
	};
	using LLAddrRegister = Register<LLAddrRegisterLayout, U32>;

	struct TagLoRegisterLayout {
		enum class Field { PTagLo,PState };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
				case Field::PState: return { 6, 7 };
				case Field::PTagLo: return { 8, 27 };
			}
		}
	};
	using TagLoRegister = Register<TagLoRegisterLayout, U32>;

	struct TagHiRegisterLayout {
		enum class Field { Value };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::Value: return { 0, 31 };
			}
		}
	};
	using TagHiRegister = Register<TagHiRegisterLayout, U32>;

	struct ContextRegisterLayout {
		enum class Field { BadVPN2, PTEBase };

		static constexpr U64 Mask = 0xFFFFFFFFFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::BadVPN2: return { 4, 22 };
			case Field::PTEBase: return { 23, 63 };
			}
		}
	};
	using ContextRegister = Register<ContextRegisterLayout, U64>;

	struct BadVAddrRegisterLayout {
		enum class Field { Value };

		static constexpr U64 Mask = 0xFFFFFFFFFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::Value: return { 0, 63 };
			}
		}
	};
	using BadVAddrRegister = Register<BadVAddrRegisterLayout, U64>;

	struct CountRegisterLayout {
		enum class Field { Value };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::Value: return { 0, 31 };
			}
		}
	};
	using CountRegister = Register<CountRegisterLayout, U32>;

	struct CompareRegisterLayout {
		enum class Field { Value };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::Value: return { 0, 31 };
			}
		}
	};
	using CompareRegister = Register<CompareRegisterLayout, U32>;

	struct StatusRegisterLayout {
		enum class Field { IE, EXL, ERL, KSU, UX, SX, KX, IM, DE, CR, CH, SR, TS, BEV, ITS, RE, FR, RP, CU };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::IE: return { 0, 1 };
			case Field::EXL: return { 1, 2 };
			case Field::ERL: return { 2, 3 };
			case Field::KSU: return { 3, 4 };
			case Field::UX: return { 4, 5 };
			case Field::SX: return { 5, 6 };
			case Field::KX: return { 6, 7 };
			case Field::IM: return { 8, 15 };
			case Field::DE: return { 15, 16 };
			case Field::CR: return { 16, 17 };
			case Field::CH: return { 17, 18 };
			case Field::SR: return { 19, 20 };
			case Field::TS: return { 20, 21 };
			case Field::BEV: return { 21, 22 };
			case Field::ITS: return { 23, 24 };
			case Field::RE: return { 25, 26 };
			case Field::FR: return { 26, 27 };
			case Field::RP: return { 27, 28 };
			case Field::CU: return { 28, 31 };
			}
		}
	};
	using StatusRegister = Register<StatusRegisterLayout, U32>;

	struct CauseRegisterLayout {
		enum class Field { ExcCode, IP, CE, BD };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::ExcCode: return { 2, 6 };
			case Field::IP: return { 8, 15 };
			case Field::CE: return { 28, 29 };
			case Field::BD: return { 30, 31 };
			}
		}
	};
	using CauseRegister = Register<CauseRegisterLayout, U32>;

	struct EPCRegisterLayout {
		enum class Field { Value };

		static constexpr U64 Mask = 0xFFFFFFFFFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::Value: return { 0, 31 };
			}
		}
	};
	using EPCRegister = Register<EPCRegisterLayout, U64>;

	struct WatchLoRegisterLayout {
		enum class Field { W,R,PAddr0 };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::W: return { 0, 1 };
			case Field::R: return { 1, 2 };
			case Field::PAddr0: return { 3, 31 };
			}
		}
	};
	using WatchLoRegister = Register<WatchLoRegisterLayout, U32>;

	struct WatchHiRegisterLayout {
		enum class Field { PAddr1 };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::PAddr1: return { 0, 3 };
			}
		}
	};
	using WatchHiRegister = Register<WatchHiRegisterLayout, U32>;

	struct XContextRegisterLayout {
		enum class Field { BadVPN2, R, PTEBase };

		static constexpr U64 Mask = 0xFFFFFFFFFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::BadVPN2: return { 4, 30 };
			case Field::R: return { 31, 32 };
			case Field::PTEBase: return { 33, 63 };
			}
		}
	};
	using XContextRegister = Register<XContextRegisterLayout, U64>;

	struct PErrRegisterLayout {
		enum class Field { Diagnostic };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::Diagnostic: return { 0, 7 };
			}
		}
	};
	using PErrRegister = Register<PErrRegisterLayout, U32>;

	struct CacheErrRegisterLayout {
		enum class Field { Value };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::Value: return { 0, 31 };
			}
		}
	};
	using CacheErrRegister = Register<CacheErrRegisterLayout, U32>;

	struct ErrorEPCRegisterLayout {
		enum class Field { Value };

		static constexpr U64 Mask = 0xFFFFFFFFFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::Value: return { 0, 63 };
			}
		}
	};
	using ErrorEPCRegister = Register<ErrorEPCRegisterLayout, U64>;

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

	class SystemControlCoprocessor : public Coprocessor {
	public:
		SystemControlCoprocessor();
		~SystemControlCoprocessor() = default;

		virtual void clock(U64 clocks) override;

		virtual void CO() override;

		void TLBR();
		void TLBWI();
		void TLBWR();
		void TLBP();
		void ERET();

		void handleInterrupts();
		void raiseException(ExceptionType type);

		U32 AddressTranslation(U32 virtualAddress);

		virtual U64 getRegister(RegisterIndex reg) const override;
		virtual void setRegister(RegisterIndex reg, U64 value) override;

		void clearInterrupt(Interrupt interrupt);
		void generateInterrupt(Interrupt interrupt);
		OperatingMode getCurrentOperatingMode() const;
		BIT is64BitMode() const;
		BIT isCoprocessorUsable(U8 copNumber) const;
		BIT isReserved64BitInstruction() const;
		BIT areInterruptsPending() const;
		BIT areInterruptsEnabled() const;
	private:
		IndexRegister      mIndexRegister;
		RandomRegister     mRandomRegister;
		EntryLo0Register   mEntryLo0Register;
		EntryLo1Register   mEntryLo1Register;
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
	};
}
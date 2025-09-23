#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	enum class CIC : U8 {
		Chip6101,
		Chip6102,
		Chip6103,
		Chip6105,
		Chip6106,
		Chip7101,
		Chip7102,
		Chip7103,
		Chip7105,
		Chip7106
	};

	enum class PIF_CommandBits : U8 {
		ConfigureJoybusFrame = 1 << 0,
		Challenge = 1 << 1,
		JoybusFlagBit = 1 << 2,
		TerminateBootProcess = 1 << 3,
		ROMLockout = 1 << 4,
		AcquireChecksum = 1 << 5,
		RunChecksum = 1 << 6,
		Complete = 1 << 7
	};


	class SIExternalBus : public BusDevice {
		friend class MemoryEditorPanel;
	public:
		SIExternalBus(StringView path);
		~SIExternalBus();

		void init() override;

		void load(const StringView& busName, U32 address, U32& output, U8 lowerBits, U8 accessSize) override;
		void store(const StringView& busName, U32 address, U32 value, U8 lowerBits, U8 accessSize) override;

		void reset();

		void setCIC(CIC cic);
	private:
		CIC mCIC;
		Vector<U8> mPIF_RAM;
		Vector<U8> mPIF_ROM;
		StringView mPIF_Path;

		BIT mLockPIF_ROM;
		U64 mAcquiredChecksum;
	};

}
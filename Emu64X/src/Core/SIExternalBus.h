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

	class SIExternalBus : public BusDevice {
		friend class MemoryEditorPanel;
	public:
		SIExternalBus(StringView path);
		~SIExternalBus();

		void init() override;

		void load(const StringView& busName, U32 address, U32& output) override;
		void store(const StringView& busName, U32 address, U32 value) override;

		void reset();

		void setCIC(CIC cic);
	private:
		CIC mCIC;
		Vector<U8> mPIF_RAM;
		Vector<U8> mPIF_ROM;
		StringView mPIF_Path;
	};

}
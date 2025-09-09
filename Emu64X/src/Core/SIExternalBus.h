#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {


	class SIExternalBus : public BusDevice {
		friend class MemoryEditorPanel;
	public:
		SIExternalBus(StringView path);
		~SIExternalBus();

		void init() override;

		void load(const StringView& busName, U32 address, U32& output) override;

		void reset();
	private:
		Vector<U8> mPIF_RAM;
		Vector<U8> mPIF_ROM;
		StringView mPIF_Path;
	};

}
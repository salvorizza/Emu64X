#pragma once

#include <string_view>
#include <vector>
#include <fstream>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	class RDRAM : public BusDevice {
	public:
		friend class MemoryEditorPanel;
		friend class DisassemblerPanel;

		RDRAM();
		~RDRAM();

		virtual void init() override;

		virtual void store(const StringView& busName, U32 address, U32 value) override;
		virtual void load(const StringView& busName, U32 address, U32& output) override;

		virtual void reset() override;
	private:
		Vector<U8> mMemory;
	};

}
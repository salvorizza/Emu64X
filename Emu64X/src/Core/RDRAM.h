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
		friend class PeripheralInterface;

		RDRAM();
		~RDRAM();

		virtual void init() override;

		virtual void store(const StringView& busName, U32 address, U32 value, U8 lowerBits, U8 accessSize) override;
		virtual void load(const StringView& busName, U32 address, U32& output, U8 lowerBits, U8 accessSize) override;

		virtual void reset() override;

	private:
		inline U32 generateMask(U8 lowerBits, U8 accessSize, U32& output) const;
	private:
		Vector<U8> mMemory;
	};

}
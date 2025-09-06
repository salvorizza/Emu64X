#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

#include "Core/MIPS/R4000/R4000.h"
#include "ScalarUnit.h"
#include "VectorUnit.h"

namespace esx {

	class RSP : public BusDevice {
	public:
		RSP();
		~RSP();

		void clock(U64 clocks) override;

		void init() override;

		void store(const StringView& busName, U32 address, U32 value) override;
		void load(const StringView& busName, U32 address, U32& output) override;

		void reset() override;
	private:
		SharedPtr<R4000> mCore;
		SharedPtr<ScalarUnit> mSU;
		SharedPtr<VectorUnit> mVU;
	};

}
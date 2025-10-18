#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

#include "Core/MIPS/R4000/R4000.h"
#include "ScalarUnit.h"
#include "VectorUnit.h"

namespace esx {

	class RCP;

	class RSP {
	public:
		RSP(RCP* rcp);
		~RSP();

		void clock(U64 clocks);

		void init();

		void store(U32 address, U32 value);
		U32 load(U32 address);

		void reset() ;

		SharedPtr<R4000>& getCore() { return mCore; }
	private:
		StringView mName = "RSP";

		RCP* mRCP;

		U64 mRCPClocks = 0;

		SharedPtr<R4000> mCore;
		SharedPtr<ScalarUnit> mSU;
		SharedPtr<VectorUnit> mVU;
	};

}
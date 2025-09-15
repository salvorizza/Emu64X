#pragma once

#include "Base/Base.h"

#define MI_VERSION_FIELDS(M) \
    M(RSP_VERSION,24,31) M(RDP_VERSION,16,23) M(RAC_VERSION,8,15) M(IO_VERSION,0,7)

#include "Base/Bus.h"

namespace esx {

	DEFINE_REGISTER_LAYOUT(MI_VERSION_Register, U32, MI_VERSION_FIELDS)

	enum class InterruptType : U8 {
		SP,
		SI,
		AI,
		VI,
		PI,
		DP
	};

	class RCP;

	class MIPSInterface {
	public:
		MIPSInterface(RCP* rcp);
		~MIPSInterface();

		void init();

		void clock(U64 clocks);

		void store(U32 address, U32 value);
		U32 load(U32 address);

		void reset();

		void setInterrupt(InterruptType type, BIT prevValue, BIT newValue, U64 delay);
		void clearInterrupt(InterruptType type);
	private:
		void setInterruptMask(U32 value);
		U32 getInterruptMask();

		U32 getInterruptStatus();

	private:
		StringView mName = "MIPSInterface";

		RCP* mRCP;

		U16 mInterruptMask = 0;
		U16 mInterruptStatus = 0;
		MI_VERSION_Register MI_VERSION;

		Vector<Pair<U64, InterruptType>> mDelayedInterrupts;
	};

}
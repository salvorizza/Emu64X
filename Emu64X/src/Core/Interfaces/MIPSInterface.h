#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	enum class InterruptType : U8 {
		SP,
		SI,
		AI,
		VI,
		PI,
		DP
	};

	class MIPSInterface : public BusDevice {
	public:
		MIPSInterface();
		~MIPSInterface();

		virtual void clock(U64 clocks) override;

		virtual void store(const StringView& busName, U32 address, U32 value) override;
		virtual void load(const StringView& busName, U32 address, U32& output) override;

		virtual void reset() override;

		void setInterrupt(InterruptType type, BIT prevValue, BIT newValue, U64 delay);
		void clearInterrupt(InterruptType type);
	private:
		void setInterruptMask(U32 value);
		U32 getInterruptMask();

		U32 getInterruptStatus();

	private:
		U16 mInterruptMask = 0;
		U16 mInterruptStatus = 0;
		Vector<Pair<U64, InterruptType>> mDelayedInterrupts;
	};

}
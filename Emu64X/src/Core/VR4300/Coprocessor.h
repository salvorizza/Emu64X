#pragma once

#include "Base/Base.h"

namespace esx {

	class VR4300;

	class Coprocessor {
	public:
		Coprocessor(VR4300* cpu, U8 number);
		virtual ~Coprocessor() = default;

		virtual void clock(U64 clocks);

		virtual void NA();
		virtual void MF();
		virtual void DMF();
		virtual void CF();
		virtual void MT();
		virtual void DMT();
		virtual void CT();
		virtual void BCF();
		virtual void BCT();
		virtual void BCFL();
		virtual void BCTL();
		virtual void CO();

		virtual U64 getRegister(RegisterIndex reg) const = 0;
		virtual void setRegister(RegisterIndex reg, U64 value) = 0;

	private:
		inline void addPendingLoad(RegisterIndex index, U64 value);
		inline void resetPendingLoad();

	protected:
		VR4300* mCPU;
	private:
		U8 mNumber;
		Pair<RegisterIndex, U64> mPendingLoad;
		Pair<RegisterIndex, U64> mMemoryLoad;
	};

}
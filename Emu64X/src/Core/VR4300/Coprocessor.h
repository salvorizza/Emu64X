#pragma once

#include "Base/Base.h"

namespace esx {

	class VR4300;

	class Coprocessor {
	public:
		Coprocessor(U8 number);
		virtual ~Coprocessor() = default;

		virtual void clock(U64 clocks);

		virtual void NA(VR4300* cpu);
		virtual void MF(VR4300* cpu);
		virtual void DMF(VR4300* cpu);
		virtual void CF(VR4300* cpu);
		virtual void MT(VR4300* cpu);
		virtual void DMT(VR4300* cpu);
		virtual void CT(VR4300* cpu);
		virtual void BCF(VR4300* cpu);
		virtual void BCT(VR4300* cpu);
		virtual void BCFL(VR4300* cpu);
		virtual void BCTL(VR4300* cpu);
		virtual void CO(VR4300* cpu);

		virtual U64 getRegister(RegisterIndex reg) const = 0;
		virtual void setRegister(RegisterIndex reg, U64 value) = 0;

	private:
		inline void addPendingLoad(RegisterIndex index, U64 value);
		inline void resetPendingLoad();

	private:
		U8 mNumber;
		Pair<RegisterIndex, U64> mPendingLoad;
		Pair<RegisterIndex, U64> mMemoryLoad;
	};

}
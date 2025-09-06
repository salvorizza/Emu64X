#pragma once

#include "Base/Base.h"

#include "Core/MIPS/Common/Coprocessor.h"

namespace esx {

	class R4000;
	class RSP;

	class VectorUnit : public Coprocessor<R4000> {
	public:
		friend class RSP;

		VectorUnit(R4000* cpu);
		~VectorUnit() = default;

		void clock(U64 clocks) override;

		void CO() override;

		void unusable() override;
		void reserved() override;

		virtual U64 getRegister(RegisterIndex reg) override;
		virtual void setRegister(RegisterIndex reg, U64 value) override;
	private:
	};
}
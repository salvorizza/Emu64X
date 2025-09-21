#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

#include "../Common/Coprocessor.h"

namespace esx {

	class VR4300;

	class FloatingPointUnit : public Coprocessor<VR4300> {
	public:
		FloatingPointUnit(VR4300* cpu);
		~FloatingPointUnit() = default;

		void clock(U64 clocks) override;

		void CO() override;

		void ADD();
		void SUB();
		void MUL();
		void DIV();
		void SQRT();
		void ABS();
		void MOV();
		void NEG();
		void ROUNDL();
		void TRUNCL();
		void CEILL();
		void FLOORL();
		void ROUNDW();
		void TRUNCW();
		void CEILW();
		void FLOORW();
		void CVTS();
		void CVTD();
		void CVTW();
		void CVTL();
		void C();

		void unusable() override;
		void reserved() override;

		U64 getRegister(RegisterIndex reg) override;
		void setRegister(RegisterIndex reg, U64 value) override;
	private:
	};
}
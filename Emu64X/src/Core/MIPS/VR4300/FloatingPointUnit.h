#pragma once

#include "Base/Base.h"

#define FCR0_FIELDS(M)  M(Rev, 0, 7)  M(Imp, 8, 15)
#define FCR31_FIELDS(M)  M(RM, 0, 1)  M(Flags, 2, 6)  M(Enables, 7, 11)  M(Cause, 12, 17)  M(C, 23, 23)  M(FS, 24, 24)

#include "Base/Bus.h"

#include "../Common/Coprocessor.h"

namespace esx {

	class VR4300;

	DEFINE_REGISTER_LAYOUT(FCR0Register, U32, FCR0_FIELDS)
	DEFINE_REGISTER_LAYOUT(FCR31Register, U32, FCR31_FIELDS)

	class FloatingPointUnit : public Coprocessor<VR4300> {
	public:
		FloatingPointUnit(VR4300* cpu);
		~FloatingPointUnit() = default;

		void clock(U64 clocks) override;

		void CF() override;
		void CT() override;
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
		FCR0Register FCR0;
		FCR31Register FCR31;

	};
}
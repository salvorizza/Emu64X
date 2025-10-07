#pragma once

#include "Base/Base.h"

#include "Core/MIPS/Common/Coprocessor.h"

namespace esx {

	class R4000;
	class RSP;

	using VectorUnitRegisterLane = U16;
	using VectorUnitAccumulatorLane = U64;

	using VectorUnitRegister = Array<VectorUnitRegisterLane, 8>;
	using VectorUnitAccumulator = Array<U64, 8>;

	class VectorUnit : public Coprocessor<R4000> {
	public:
		friend class RSP;

		VectorUnit(R4000* cpu);
		~VectorUnit() = default;

		void clock(U64 clocks) override;

		void MF() override;
		void CF() override;
		void MT() override;
		void CT() override;
		void CO() override;

		void VMULF();
		void VMULU();
		void VMUDL();
		void VMUDM();
		void VMUDN();
		void VMUDH();
		void VMACF();
		void VMACU();
		void VMADL();
		void VMADM();
		void VMADN();
		void VMADH();
		void VADD();
		void VABS();
		void VADDC();
		void VSAR();
		void VAND();
		void VNAND();
		void VOR();
		void VNOR();
		void VXOR();
		void VNXOR();
		void VLT();
		void VEQ();
		void VNE();
		void VGE();
		void VCL();
		void VCH();
		void VCR();
		void VMRG();

		void unusable() override;
		void reserved() override;

		virtual U64 getRegister(RegisterIndex reg) override;
		virtual void setRegister(RegisterIndex reg, U64 value) override;

		void setVPRRegisterBytes(U8 vt, U64 data, U8 element, size_t access_size);
		U64 getVPRRegisterBytes(U8 vt, U8 element, size_t access_size);

	private:
		Array<VectorUnitRegister, 32> VPR;
		VectorUnitRegister ACCUM;
	};

	typedef void (VectorUnit::* VectorOpFunc)();
}
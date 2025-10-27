#pragma once

#include "Base/Base.h"

#define VU_LOAD_STORE_INSTRUCTION_FIELDS(M) \
    M(offset,0,6)              \
    M(element,7,10)              \
    M(opcode,11,15)              \
    M(vt,16,20)              \
    M(base,21,25)

#define VU_SL_INSTRUCTION_FIELDS(M) \
    M(opcode,0,5)              \
    M(vd,6,10)              \
    M(vd_elem,11,15)              \
    M(vt,16,20)              \
    M(vt_elem,21,24)

#define VU_COMPUTATIONAL_INSTRUCTION_FIELDS(M) \
    M(opcode,0,5)              \
    M(vd,6,10)              \
    M(vs,11,15)              \
    M(vt,16,20)              \
    M(element,21,24)

#define VU_SELECT_INSTRUCTION_FIELDS(M) \
    M(opcode,0,5)              \
    M(vd,6,10)              \
    M(vs,11,15)              \
    M(vt,16,20)              \
    M(element,21,24)

#define VU_MOVE_INSTRUCTION_FIELDS(M) \
    M(vs_elem,8,10)              \
    M(vs,11,15)              \
    M(rt,16,20)              

#define VU_ACCUM_FIELDS(M)		 \
    M(ACCUM_LO,0,15)              \
    M(ACCUM_MD,16,31)              \
    M(ACCUM_HI,32,47)				\
	M(ACCUM_D,0,31)					\
	M(ACCUM_M,16,47)				 \
	M(ACCUM,0,47)

#include "Core/MIPS/Common/Coprocessor.h"

namespace esx {
	DEFINE_REGISTER_LAYOUT(VU_LOAD_STORE_INSTRUCTION_Register, U32, VU_LOAD_STORE_INSTRUCTION_FIELDS)
	DEFINE_REGISTER_LAYOUT(VU_SL_INSTRUCTION_Register, U32, VU_SL_INSTRUCTION_FIELDS)
	DEFINE_REGISTER_LAYOUT(VU_COMPUTATIONAL_INSTRUCTION_Register, U32, VU_COMPUTATIONAL_INSTRUCTION_FIELDS)
	DEFINE_REGISTER_LAYOUT(VU_SELECT_INSTRUCTION_Register, U32, VU_SELECT_INSTRUCTION_FIELDS)
	DEFINE_REGISTER_LAYOUT(VU_MOVE_INSTRUCTION_Register, U32, VU_MOVE_INSTRUCTION_FIELDS)
	DEFINE_REGISTER_LAYOUT(VU_ACCUM_Register, U64, VU_ACCUM_FIELDS)

	class R4000;
	class RSP;

	using VectorUnitRegisterLane = U16;
	using VectorUnitAccumulatorLane = VU_ACCUM_Register;

	using VectorUnitRegister = Array<VectorUnitRegisterLane, 8>;
	using VectorUnitAccumulator = Array<VectorUnitAccumulatorLane, 8>;

	class VectorUnit : public Coprocessor<R4000> {
	public:
		friend class CPUStatusPanel;
		friend class RSP;

		VectorUnit(R4000* cpu);
		~VectorUnit() = default;

		void clock(U64 clocks) override;

		void MF() override;
		void CF() override;
		void MT() override;
		void CT() override;
		void CO() override;

		// 0x00-0x07
		void VMULF();
		void VMULU();
		void VRNDP();
		void VMULQ();
		void VMUDL();
		void VMUDM();
		void VMUDN();
		void VMUDH();

		// 0x08-0x0F
		void VMACF();
		void VMACU();
		void VRNDN();
		void VMACQ();
		void VMADL();
		void VMADM();
		void VMADN();
		void VMADH();

		// 0x10-0x17
		void VADD();
		void VSUB();
		void VSUT();
		void VABS();
		void VADDC();
		void VSUBC();
		void VADDB();
		void VSUBB();

		// 0x18-0x1F
		void VACCB();
		void VSUCB();
		void VSAD();
		void VSAC();
		void VSUM();
		void VSAW();

		// 0x20-0x27
		void VLT();
		void VEQ();
		void VNE();
		void VGE();
		void VCL();
		void VCH();
		void VCR();
		void VMRG();

		// 0x28-0x2F
		void VAND();
		void VNAND();
		void VOR();
		void VNOR();
		void VXOR();
		void VNXOR();

		// 0x30-0x37
		void VRCP();
		void VRCPL();
		void VRCPH();
		void VMOV();
		void VRSQ();
		void VRSQL();
		void VRSQH();

		void unusable() override;
		void reserved() override;

		virtual U64 getRegister(RegisterIndex reg) override;
		virtual void setRegister(RegisterIndex reg, U64 value) override;

		void setVPRRegisterBytes(U8 vt, Span<U8>& data, U8 element, size_t access_size);
		Span<U8> getVPRRegisterBytes(U8 vt, U8 element, size_t access_size);

	private:
		Array<VectorUnitRegister, 32> VPR = {};
		VectorUnitAccumulator ACCUM = {};
		VectorUnitRegisterLane DIV_IN = {}, DIV_OUT = {};
		std::bitset<16> VCC = {}, VCO = {}, VCE = {};
	};

	typedef void (VectorUnit::* VectorOpFunc)();
}
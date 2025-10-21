#include "VectorUnit.h"

#include <intrin.h>

#include "Core/MIPS/R4000/R4000.h"

#pragma intrinsic(_BitScanReverse)

namespace esx {

    static Array<U16,512> RCP_ROM = {
        0xffff,  0xff00,  0xfe01,  0xfd04,  0xfc07,  0xfb0c,  0xfa11,  0xf918,  0xf81f,  0xf727,  0xf631,  0xf53b,  0xf446,  0xf352,  0xf25f,  0xf16d,
        0xf07c,  0xef8b,  0xee9c,  0xedae,  0xecc0,  0xebd3,  0xeae8,  0xe9fd,  0xe913,  0xe829,  0xe741,  0xe65a,  0xe573,  0xe48d,  0xe3a9,  0xe2c5,
        0xe1e1,  0xe0ff,  0xe01e,  0xdf3d,  0xde5d,  0xdd7e,  0xdca0,  0xdbc2,  0xdae6,  0xda0a,  0xd92f,  0xd854,  0xd77b,  0xd6a2,  0xd5ca,  0xd4f3,
        0xd41d,  0xd347,  0xd272,  0xd19e,  0xd0cb,  0xcff8,  0xcf26,  0xce55,  0xcd85,  0xccb5,  0xcbe6,  0xcb18,  0xca4b,  0xc97e,  0xc8b2,  0xc7e7,
        0xc71c,  0xc652,  0xc589,  0xc4c0,  0xc3f8,  0xc331,  0xc26b,  0xc1a5,  0xc0e0,  0xc01c,  0xbf58,  0xbe95,  0xbdd2,  0xbd10,  0xbc4f,  0xbb8f,
        0xbacf,  0xba10,  0xb951,  0xb894,  0xb7d6,  0xb71a,  0xb65e,  0xb5a2,  0xb4e8,  0xb42e,  0xb374,  0xb2bb,  0xb203,  0xb14b,  0xb094,  0xafde,
        0xaf28,  0xae73,  0xadbe,  0xad0a,  0xac57,  0xaba4,  0xaaf1,  0xaa40,  0xa98e,  0xa8de,  0xa82e,  0xa77e,  0xa6d0,  0xa621,  0xa574,  0xa4c6,
        0xa41a,  0xa36e,  0xa2c2,  0xa217,  0xa16d,  0xa0c3,  0xa01a,  0x9f71,  0x9ec8,  0x9e21,  0x9d79,  0x9cd3,  0x9c2d,  0x9b87,  0x9ae2,  0x9a3d,
        0x9999,  0x98f6,  0x9852,  0x97b0,  0x970e,  0x966c,  0x95cb,  0x952b,  0x948b,  0x93eb,  0x934c,  0x92ad,  0x920f,  0x9172,  0x90d4,  0x9038,
        0x8f9c,  0x8f00,  0x8e65,  0x8dca,  0x8d30,  0x8c96,  0x8bfc,  0x8b64,  0x8acb,  0x8a33,  0x899c,  0x8904,  0x886e,  0x87d8,  0x8742,  0x86ad,
        0x8618,  0x8583,  0x84f0,  0x845c,  0x83c9,  0x8336,  0x82a4,  0x8212,  0x8181,  0x80f0,  0x8060,  0x7fd0,  0x7f40,  0x7eb1,  0x7e22,  0x7d93,
        0x7d05,  0x7c78,  0x7beb,  0x7b5e,  0x7ad2,  0x7a46,  0x79ba,  0x792f,  0x78a4,  0x781a,  0x7790,  0x7706,  0x767d,  0x75f5,  0x756c,  0x74e4,
        0x745d,  0x73d5,  0x734f,  0x72c8,  0x7242,  0x71bc,  0x7137,  0x70b2,  0x702e,  0x6fa9,  0x6f26,  0x6ea2,  0x6e1f,  0x6d9c,  0x6d1a,  0x6c98,
        0x6c16,  0x6b95,  0x6b14,  0x6a94,  0x6a13,  0x6993,  0x6914,  0x6895,  0x6816,  0x6798,  0x6719,  0x669c,  0x661e,  0x65a1,  0x6524,  0x64a8,
        0x642c,  0x63b0,  0x6335,  0x62ba,  0x623f,  0x61c5,  0x614b,  0x60d1,  0x6058,  0x5fdf,  0x5f66,  0x5eed,  0x5e75,  0x5dfd,  0x5d86,  0x5d0f,
        0x5c98,  0x5c22,  0x5bab,  0x5b35,  0x5ac0,  0x5a4b,  0x59d6,  0x5961,  0x58ed,  0x5879,  0x5805,  0x5791,  0x571e,  0x56ac,  0x5639,  0x55c7,
        0x5555,  0x54e3,  0x5472,  0x5401,  0x5390,  0x5320,  0x52af,  0x5240,  0x51d0,  0x5161,  0x50f2,  0x5083,  0x5015,  0x4fa6,  0x4f38,  0x4ecb,
        0x4e5e,  0x4df1,  0x4d84,  0x4d17,  0x4cab,  0x4c3f,  0x4bd3,  0x4b68,  0x4afd,  0x4a92,  0x4a27,  0x49bd,  0x4953,  0x48e9,  0x4880,  0x4817,
        0x47ae,  0x4745,  0x46dc,  0x4674,  0x460c,  0x45a5,  0x453d,  0x44d6,  0x446f,  0x4408,  0x43a2,  0x433c,  0x42d6,  0x4270,  0x420b,  0x41a6,
        0x4141,  0x40dc,  0x4078,  0x4014,  0x3fb0,  0x3f4c,  0x3ee8,  0x3e85,  0x3e22,  0x3dc0,  0x3d5d,  0x3cfb,  0x3c99,  0x3c37,  0x3bd6,  0x3b74,
        0x3b13,  0x3ab2,  0x3a52,  0x39f1,  0x3991,  0x3931,  0x38d2,  0x3872,  0x3813,  0x37b4,  0x3755,  0x36f7,  0x3698,  0x363a,  0x35dc,  0x357f,
        0x3521,  0x34c4,  0x3467,  0x340a,  0x33ae,  0x3351,  0x32f5,  0x3299,  0x323e,  0x31e2,  0x3187,  0x312c,  0x30d1,  0x3076,  0x301c,  0x2fc2,
        0x2f68,  0x2f0e,  0x2eb4,  0x2e5b,  0x2e02,  0x2da9,  0x2d50,  0x2cf8,  0x2c9f,  0x2c47,  0x2bef,  0x2b97,  0x2b40,  0x2ae8,  0x2a91,  0x2a3a,
        0x29e4,  0x298d,  0x2937,  0x28e0,  0x288b,  0x2835,  0x27df,  0x278a,  0x2735,  0x26e0,  0x268b,  0x2636,  0x25e2,  0x258d,  0x2539,  0x24e5,
        0x2492,  0x243e,  0x23eb,  0x2398,  0x2345,  0x22f2,  0x22a0,  0x224d,  0x21fb,  0x21a9,  0x2157,  0x2105,  0x20b4,  0x2063,  0x2012,  0x1fc1,
        0x1f70,  0x1f1f,  0x1ecf,  0x1e7f,  0x1e2e,  0x1ddf,  0x1d8f,  0x1d3f,  0x1cf0,  0x1ca1,  0x1c52,  0x1c03,  0x1bb4,  0x1b66,  0x1b17,  0x1ac9,
        0x1a7b,  0x1a2d,  0x19e0,  0x1992,  0x1945,  0x18f8,  0x18ab,  0x185e,  0x1811,  0x17c4,  0x1778,  0x172c,  0x16e0,  0x1694,  0x1648,  0x15fd,
        0x15b1,  0x1566,  0x151b,  0x14d0,  0x1485,  0x143b,  0x13f0,  0x13a6,  0x135c,  0x1312,  0x12c8,  0x127f,  0x1235,  0x11ec,  0x11a3,  0x1159,
        0x1111,  0x10c8,  0x107f,  0x1037,  0x0fef,  0x0fa6,  0x0f5e,  0x0f17,  0x0ecf,  0x0e87,  0x0e40,  0x0df9,  0x0db2,  0x0d6b,  0x0d24,  0x0cdd,
        0x0c97,  0x0c50,  0x0c0a,  0x0bc4,  0x0b7e,  0x0b38,  0x0af2,  0x0aad,  0x0a68,  0x0a22,  0x09dd,  0x0998,  0x0953,  0x090f,  0x08ca,  0x0886,
        0x0842,  0x07fd,  0x07b9,  0x0776,  0x0732,  0x06ee,  0x06ab,  0x0668,  0x0624,  0x05e1,  0x059e,  0x055c,  0x0519,  0x04d6,  0x0494,  0x0452,
        0x0410,  0x03ce,  0x038c,  0x034a,  0x0309,  0x02c7,  0x0286,  0x0245,  0x0204,  0x01c3,  0x0182,  0x0141,  0x0101,  0x00c0,  0x0080,  0x0040
    };

    U16 clamp_unsigned(I32 accum) {
        return std::clamp<I32>(accum, 0, std::numeric_limits<U16>::max());
    }

    U16 clamp_signed(I32 accum) {
        return std::clamp<I32>(accum, std::numeric_limits<I16>::min(), std::numeric_limits<I16>::max());
    }

    Pair<U8, U8> calculate_se_de(unsigned long vd_elem, unsigned long vt_elem) {
        U8 de = 0, se = 0;
        unsigned long msb = 0;

        de = vd_elem & 0x7;
        unsigned char found = _BitScanReverse(&msb, vt_elem);
        if (!found)
            msb = 0;
        
        unsigned long mask = (0xF << msb) & 0xF;
        se = ((vd_elem & mask) | (vt_elem & ~mask)) & 0x7;

        return std::make_pair(se, de);
    }

    U32 rcp(I32 input) {
        U32 result = 0;
        unsigned long scale_out = 0;

        if (input == 0)
            return ~result;

        U32 x = static_cast<U32>(input < 0 ? -input : input);

        unsigned char found = _BitScanReverse(&scale_out, x);
        if (!found)
            scale_out = 0;

        unsigned long scale_in = 32 - scale_out;

        U32 rom_index = (x >> (scale_in - 1)) & 0x1FF;
        result = ((1u << 16) | RCP_ROM[rom_index]) << scale_out;

        if (input < 0)
            result = ~result;

        return result;
    }

    VectorUnit::VectorUnit(R4000* cpu)
        : Coprocessor(cpu, 2)
    {
    }

    void VectorUnit::clock(U64 clocks)
    {

    }

    void VectorUnit::MF()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::CF()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::MT()
    {
        RegisterIndex rt = mCPU->mCurrentInstruction.RegisterTarget();
        U8 vs = mCPU->mCurrentInstruction.RegisterDestination().Value;
        U8 vs_elem = mCPU->mCurrentInstruction.Element();

        U16 data = mCPU->getRegister(rt);
        setVPRRegisterBytes(vs, data, vs_elem, vs_elem == 15 ? 1 : 2);
    }

    void VectorUnit::CT()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }


    static const Array<VectorOpFunc, 64> vuOpcodeTable = {
        &VectorUnit::VMULF,  &VectorUnit::VMULU,  &VectorUnit::VRNDP,  &VectorUnit::VMULQ,  &VectorUnit::VMUDL,  &VectorUnit::VMUDM,  &VectorUnit::VMUDN,  &VectorUnit::VMUDH,  // 0x00-0x07
        &VectorUnit::VMACF,  &VectorUnit::VMACU,  &VectorUnit::VRNDN,  &VectorUnit::VMACQ,  &VectorUnit::VMADL,  &VectorUnit::VMADM,  &VectorUnit::VMADN,  &VectorUnit::VMADH,  // 0x08-0x0F
        &VectorUnit::VADD,   &VectorUnit::VSUB,   &VectorUnit::VSUT,   &VectorUnit::VABS,   &VectorUnit::VADDC,  &VectorUnit::VSUBC,  &VectorUnit::VADDB,  &VectorUnit::VSUBB,  // 0x10-0x17
        &VectorUnit::VACCB,  &VectorUnit::VSUCB,  &VectorUnit::VSAD,   &VectorUnit::VSAC,   &VectorUnit::VSUM,   &VectorUnit::VSAW,   &VectorUnit::NA,     &VectorUnit::NA,     // 0x18-0x1F
        &VectorUnit::VLT,    &VectorUnit::VEQ,    &VectorUnit::VNE,    &VectorUnit::VGE,    &VectorUnit::VCL,    &VectorUnit::VCH,    &VectorUnit::VCR,    &VectorUnit::VMRG,   // 0x20-0x27
        &VectorUnit::VAND,   &VectorUnit::VNAND,  &VectorUnit::VOR,    &VectorUnit::VNOR,   &VectorUnit::VXOR,   &VectorUnit::VNXOR,  &VectorUnit::NA,     &VectorUnit::NA,     // 0x28-0x2F
        &VectorUnit::VRCP,   &VectorUnit::VRCPL,  &VectorUnit::VRCPH,  &VectorUnit::VMOV,   &VectorUnit::VRSQ,   &VectorUnit::VRSQL,  &VectorUnit::VRSQH,  &VectorUnit::NA,     // 0x30-0x37
        &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA      // 0x38-0x3F
    };

    void VectorUnit::CO()
    {
        auto instruction = vuOpcodeTable[mCPU->mCurrentInstruction.Function()];
        (this->*instruction)();
    }

    void VectorUnit::VMULF()
    {
        VU_COMPUTATIONAL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 element = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::element).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        RegisterIndex vs = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vs).as<RegisterIndex>();
        RegisterIndex vd = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        for (I32 i = 0; i < 8; i++) {
            I64 prod = static_cast<I32>(static_cast<I16>(VPR[vs][i]) * static_cast<I16>(VPR[vt][i]) * 2);
            ACCUM[i].write(static_cast<I64>(static_cast<I32>(prod) + 0x8000));
            VPR[vd][i] = clamp_signed(static_cast<I32>(ACCUM[i].read() >> 16));
        }
    }

    void VectorUnit::VMULU()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VRNDP()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMULQ()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMUDL()
    {
        VU_COMPUTATIONAL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 element = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::element).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        RegisterIndex vs = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vs).as<RegisterIndex>();
        RegisterIndex vd = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        for (I32 i = 0; i < 8; i++) {
            U32 prod = VPR[vs][i] * VPR[vt][i];
            ACCUM[i].write(0);
            ACCUM[i].write(ACCUM[i].read() + static_cast<I64>(static_cast<I32>(prod >> 16)));
            VPR[vd][i] = clamp_unsigned(static_cast<I32>(static_cast<U32>(ACCUM[i].read())));
        }
    }

    void VectorUnit::VMUDM()
    {
        VU_COMPUTATIONAL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 element = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::element).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        RegisterIndex vs = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vs).as<RegisterIndex>();
        RegisterIndex vd = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        for (I32 i = 0; i < 8; i++) {
            U32 prod = VPR[vs][i] * VPR[vt][i];
            ACCUM[i].write(0);
            ACCUM[i].write(ACCUM[i].read() + static_cast<I64>(static_cast<I32>(prod)));
            VPR[vd][i] = clamp_signed(static_cast<I32>(static_cast<U32>(ACCUM[i].read())));
        }
    }

    void VectorUnit::VMUDN()
    {
        VU_COMPUTATIONAL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 element = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::element).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        RegisterIndex vs = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vs).as<RegisterIndex>();
        RegisterIndex vd = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        for (I32 i = 0; i < 8; i++) {
            U32 prod = VPR[vs][i] * VPR[vt][i];
            ACCUM[i].write(0);
            ACCUM[i].write(ACCUM[i].read() + static_cast<I64>(static_cast<I32>(prod)));
            VPR[vd][i] = clamp_unsigned(static_cast<I32>(static_cast<U32>(ACCUM[i].read())));
        }
    }

    void VectorUnit::VMUDH()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMACF()
    {
        VU_COMPUTATIONAL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 element = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::element).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        RegisterIndex vs = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vs).as<RegisterIndex>();
        RegisterIndex vd = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        for (I32 i = 0; i < 8; i++) {
            I64 prod = static_cast<I32>(static_cast<I16>(VPR[vs][i]) * static_cast<I16>(VPR[vt][i]) * 2);
            ACCUM[i].write(ACCUM[i].read() + static_cast<I64>(static_cast<I32>(prod)));
            VPR[vd][i] = clamp_signed(static_cast<I32>(ACCUM[i].read() >> 16));
        }
    }

    void VectorUnit::VMACU()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VRNDN()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMACQ()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMADL()
    {
        VU_COMPUTATIONAL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 element = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::element).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        RegisterIndex vs = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vs).as<RegisterIndex>();
        RegisterIndex vd = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        for (I32 i = 0; i < 8; i++) {
            U32 prod = VPR[vs][i] * VPR[vt][i];
            ACCUM[i].write(ACCUM[i].read() + static_cast<I64>(static_cast<I32>(prod >> 16)));
            VPR[vd][i] = clamp_unsigned(static_cast<I32>(static_cast<U32>(ACCUM[i].read())));
        }
    }

    void VectorUnit::VMADM()
    {
        VU_COMPUTATIONAL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 element = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::element).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        RegisterIndex vs = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vs).as<RegisterIndex>();
        RegisterIndex vd = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        for (I32 i = 0; i < 8; i++) {
            U32 prod = VPR[vs][i] * VPR[vt][i];
            ACCUM[i].write(ACCUM[i].read() + static_cast<I64>(static_cast<I32>(prod)));
            VPR[vd][i] = clamp_signed(static_cast<I32>(static_cast<U32>(ACCUM[i].read())));
        }
    }

    void VectorUnit::VMADN()
    {
        VU_COMPUTATIONAL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 element = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::element).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        RegisterIndex vs = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vs).as<RegisterIndex>();
        RegisterIndex vd = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        for (I32 i = 0; i < 8; i++) {
            U32 prod = VPR[vs][i] * VPR[vt][i];
            ACCUM[i].write(ACCUM[i].read() + static_cast<I64>(static_cast<I32>(prod)));
            VPR[vd][i] = clamp_unsigned(static_cast<I32>(static_cast<U32>(ACCUM[i].read())));
        }
    }

    void VectorUnit::VMADH()
    {
        VU_COMPUTATIONAL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 element = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::element).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        RegisterIndex vs = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vs).as<RegisterIndex>();
        RegisterIndex vd = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        for (I32 i = 0; i < 8; i++) {
            U32 prod = VPR[vs][i] * VPR[vt][i];
            ACCUM[i].write((((ACCUM[i].read() >> 16) + prod) << 16) | ACCUM[i].read() & 0xFFFF);
            VPR[vd][i] = clamp_signed(static_cast<I32>(static_cast<U32>(ACCUM[i].read() >> 16)));
        }
    }

    void VectorUnit::VADD()
    {
        VU_COMPUTATIONAL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 element = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::element).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        RegisterIndex vs = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vs).as<RegisterIndex>();
        RegisterIndex vd = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        for (I32 i = 0; i < 8; i++) {
            I32 result = VPR[vs][i] + VPR[vt][i] + ((VCO >> i) & 0x1);
            ACCUM[i].set(layouts::VU_ACCUM_Register::Field::ACCUM_LO, static_cast<U16>(result));
            VPR[vd][i] = clamp_signed(result);
            VCO &= ~(1 << i);
            VCO &= ~(1 << (i + 8));
        }
    }

    void VectorUnit::VSUB()
    {
        VU_COMPUTATIONAL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 element = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::element).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        RegisterIndex vs = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vs).as<RegisterIndex>();
        RegisterIndex vd = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        for (I32 i = 0; i < 8; i++) {
            I32 result = VPR[vs][i] - VPR[vt][i] - ((VCO >> i) & 0x1);
            ACCUM[i].set(layouts::VU_ACCUM_Register::Field::ACCUM_LO, static_cast<U16>(result));
            VPR[vd][i] = clamp_signed(result);
            VCO &= ~(1 << i);
            VCO &= ~(1 << (i + 8));
        }
    }

    void VectorUnit::VSUT()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VABS()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VADDC()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VSUBC()
    {
        VU_COMPUTATIONAL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 element = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::element).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        RegisterIndex vs = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vs).as<RegisterIndex>();
        RegisterIndex vd = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        for (I32 i = 0; i < 8; i++) {
            U32 result = VPR[vs][i] - VPR[vt][i] - ((VCO >> i) & 0x1);
            ACCUM[i].set(layouts::VU_ACCUM_Register::Field::ACCUM_LO, static_cast<U16>(result));
            VPR[vd][i] = static_cast<U16>(result);
            VCO &= ~((result >> 16) << i);
            VCO &= ~((result != 0 ? 1 : 0) << (i + 8));
        }
    }

    void VectorUnit::VADDB()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VSUBB()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VACCB()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VSUCB()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VSAD()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VSAC()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VSUM()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VSAW()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VAND()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VNAND()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VOR()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VNOR()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VXOR()
    {
        VU_COMPUTATIONAL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 element = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::element).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        RegisterIndex vs = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vs).as<RegisterIndex>();
        RegisterIndex vd = instruction.get(layouts::VU_COMPUTATIONAL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        for (I32 i = 0; i < 8; i++) {
            U16 result = VPR[vs][i] ^ VPR[vt][i];
            ACCUM[i].set(layouts::VU_ACCUM_Register::Field::ACCUM_LO, result);
            VPR[vd][i] = result;
        }
    }

    void VectorUnit::VNXOR()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VRCP()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VRCPL()
    {
        VU_SL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 vt_elem = instruction.get(layouts::VU_SL_INSTRUCTION_Register::Field::vt_elem).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_SL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        U8 vd_elem = instruction.get(layouts::VU_SL_INSTRUCTION_Register::Field::vd_elem).as<U8>();
        RegisterIndex vd = instruction.get(layouts::VU_SL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        auto [se, de] = calculate_se_de(vd_elem, vt_elem);

        U32 result = rcp(static_cast<I32>((static_cast<U32>(DIV_IN) << 16) | VPR[vt][se]));
        VPR[vd][de] = result;
        DIV_OUT = result >> 16;
        DIV_IN = 0;
        for (I32 i = 0; i < 8; i++) {
            ACCUM[i].set(layouts::VU_ACCUM_Register::Field::ACCUM_LO, VPR[vt][i]);
        }
    }

    void VectorUnit::VRCPH()
    {
        VU_SL_INSTRUCTION_Register instruction;
        instruction.write(mCPU->mCurrentInstruction.binaryInstruction);

        U8 vt_elem = instruction.get(layouts::VU_SL_INSTRUCTION_Register::Field::vt_elem).as<U8>();
        RegisterIndex vt = instruction.get(layouts::VU_SL_INSTRUCTION_Register::Field::vt).as<RegisterIndex>();
        U8 vd_elem = instruction.get(layouts::VU_SL_INSTRUCTION_Register::Field::vd_elem).as<U8>();
        RegisterIndex vd = instruction.get(layouts::VU_SL_INSTRUCTION_Register::Field::vd).as<RegisterIndex>();

        auto [se, de] = calculate_se_de(vd_elem, vt_elem);

        VPR[vd][de] = DIV_OUT;
        DIV_IN = VPR[vt][se];
        for (I32 i = 0; i < 8; i++) {
            ACCUM[i].set(layouts::VU_ACCUM_Register::Field::ACCUM_LO, VPR[vt][i]);
        }
    }

    void VectorUnit::VMOV()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VRSQ()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VRSQL()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VRSQH()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VLT()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VEQ()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VNE()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VGE()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VCL()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VCH()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VCR()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMRG()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::unusable()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::reserved()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet {:02x}h", __FUNCTION__, mCPU->mCurrentInstruction.CoprocessorFunction());
    }

    U64 VectorUnit::getRegister(RegisterIndex reg)
    {
        return 0;
    }

    void VectorUnit::setRegister(RegisterIndex reg, U64 value)
    {
    }

    void VectorUnit::setVPRRegisterBytes(U8 vt, U64 data, U8 element, size_t access_size)
    {
        for (I32 i = 0; i < access_size; i++) {
            *(reinterpret_cast<U8*>(VPR[vt].data()) + element + i) = (data >> ((access_size - 1 - i) * 8)) & 0xFF;
        }
    }

    U64 VectorUnit::getVPRRegisterBytes(U8 vt, U8 element, size_t access_size)
    {
        U64 data = 0;

        for (I32 i = 0; i < access_size; i++) {
            data |= *(reinterpret_cast<U8*>(VPR[vt].data()) + element + i);
            data <<= 8;
        }

        return data;
    }

}
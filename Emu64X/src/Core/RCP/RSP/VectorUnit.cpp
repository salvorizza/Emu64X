#include "VectorUnit.h"

#include "Core/MIPS/R4000/R4000.h"

namespace esx {

    U16 clamp_unsigned(I32 accum) {
        return std::clamp<I32>(accum, 0, std::numeric_limits<U16>::max());
    }

    U16 clamp_signed(I32 accum) {
        return std::clamp<I32>(accum, std::numeric_limits<I16>::min(), std::numeric_limits<I16>::max());
    }

    VectorUnit::VectorUnit(R4000* cpu)
        : Coprocessor(cpu, 2),
            VPR()
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
        &VectorUnit::VMULF,  &VectorUnit::VMULU,  &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::VMUDL,  &VectorUnit::VMUDM,  &VectorUnit::VMUDN,  &VectorUnit::VMUDH,  // 0x00-0x07
        &VectorUnit::VMACF,  &VectorUnit::VMACU,  &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::VMADL,  &VectorUnit::VMADM,  &VectorUnit::VMADN,  &VectorUnit::VMADH,  // 0x08-0x0F
        &VectorUnit::VADD,   &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::VABS,   &VectorUnit::VADDC,  &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     // 0x10-0x17
        &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::VSAR,   &VectorUnit::NA,     &VectorUnit::NA,     // 0x18-0x1F
        &VectorUnit::VLT,    &VectorUnit::VEQ,    &VectorUnit::VNE,    &VectorUnit::VGE,    &VectorUnit::VCL,    &VectorUnit::VCH,    &VectorUnit::VCR,    &VectorUnit::VMRG,   // 0x20-0x27
        &VectorUnit::VAND,   &VectorUnit::VNAND,  &VectorUnit::VOR,    &VectorUnit::VNOR,   &VectorUnit::VXOR,   &VectorUnit::VNXOR,  &VectorUnit::NA,     &VectorUnit::NA,     // 0x28-0x2F
        &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     // 0x30-0x37
        &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA,     &VectorUnit::NA      // 0x38-0x3F
    };

    void VectorUnit::CO()
    {
        auto instruction = vuOpcodeTable[mCPU->mCurrentInstruction.CoprocessorFunction()];
        (this->*instruction)();
    }

    void VectorUnit::VMULF()
    {
        U8 element = mCPU->mCurrentInstruction.RegisterSource().Value & 0xF;
        RegisterIndex vt = mCPU->mCurrentInstruction.RegisterTarget();
        RegisterIndex vs = mCPU->mCurrentInstruction.RegisterDestination();
        RegisterIndex vd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

        for (I32 i = 0; i < 8; i++) {
            I64 prod = static_cast<I32>(static_cast<I16>(VPR[vs][i]) * static_cast<I16>(VPR[vt][i]) * 2);
            ACCUM[i] = static_cast<I64>(static_cast<I32>(prod) + 0x8000);
            VPR[vd][i] = clamp_signed(static_cast<I32>(ACCUM[i] >> 16));
        }
    }

    void VectorUnit::VMULU()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMUDL()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMUDM()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMUDN()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMUDH()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMACF()
    {
        U8 element = mCPU->mCurrentInstruction.RegisterSource().Value & 0xF;
        RegisterIndex vt = mCPU->mCurrentInstruction.RegisterTarget();
        RegisterIndex vs = mCPU->mCurrentInstruction.RegisterDestination();
        RegisterIndex vd = RegisterIndex(mCPU->mCurrentInstruction.ShiftAmount());

        for (I32 i = 0; i < 8; i++) {
            I64 prod = static_cast<I32>(static_cast<I16>(VPR[vs][i]) * static_cast<I16>(VPR[vt][i]) * 2);
            ACCUM[i] += static_cast<I64>(static_cast<I32>(prod));
            VPR[vd][i] = clamp_signed(static_cast<I32>(ACCUM[i] >> 16));
        }
    }

    void VectorUnit::VMACU()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMADL()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMADM()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMADN()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VMADH()
    {
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VADD()
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

    void VectorUnit::VSAR()
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
        ESX_CORE_LOG_WARNING("{} not implemented yet", __FUNCTION__);
    }

    void VectorUnit::VNXOR()
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
            *(reinterpret_cast<U8*>(&VPR[vt][7]) + 1 - element - i) = data & 0xFF;
            data >>= 8;
        }
    }

    U64 VectorUnit::getVPRRegisterBytes(U8 vt, U8 element, size_t access_size)
    {
        U64 data = 0;

        for (I32 i = 0; i < access_size; i++) {
            data |= *(reinterpret_cast<U8*>(&VPR[vt][7]) + 1 - element - i);
            data <<= 8;
        }

        return data;
    }

}
#include "VectorUnit.h"

#include "Core/MIPS/R4000/R4000.h"

namespace esx {
    VectorUnit::VectorUnit(R4000* cpu)
        : Coprocessor(cpu, 2)
    {
    }

    void VectorUnit::clock(U64 clocks)
    {

    }

    void VectorUnit::CO()
    {

    }

    void VectorUnit::unusable()
    {
    }

    void VectorUnit::reserved()
    {
    }

    U64 VectorUnit::getRegister(RegisterIndex reg)
    {
        return 0;
    }

    void VectorUnit::setRegister(RegisterIndex reg, U64 value)
    {
    }

}
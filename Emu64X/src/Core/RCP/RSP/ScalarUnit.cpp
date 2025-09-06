#include "ScalarUnit.h"

#include "Core/MIPS/R4000/R4000.h"

namespace esx {
    ScalarUnit::ScalarUnit(R4000* cpu)
        : Coprocessor(cpu, 0)
    {
    }

    void ScalarUnit::clock(U64 clocks)
    {
        
    }

    void ScalarUnit::CO()
    {
        
    }

    void ScalarUnit::unusable()
    {
    }

    void ScalarUnit::reserved()
    {
    }

    void ScalarUnit::signalBreak()
    {
        SP_STATUS.set(SP_STATUS_RegisterLayout::Field::BROKE, ESX_TRUE);
        mCPU->setHalt(ESX_TRUE);

        if (SP_STATUS.get(SP_STATUS_RegisterLayout::Field::INTBREAK).as<BIT>() == ESX_TRUE) {
            //TODO: Interrupt MI
        }
    }

    U64 ScalarUnit::getRegister(RegisterIndex reg)
    {
        switch (static_cast<ScalarUnitRegisterType>(reg.Value)) {
            case ScalarUnitRegisterType::c0:    return SP_DMA_SPADDR.read();
            case ScalarUnitRegisterType::c1:    return SP_DMA_RAMADDR.read();
            case ScalarUnitRegisterType::c2:    return SP_DMA_RDLEN.read();
            case ScalarUnitRegisterType::c3:    return SP_DMA_WRLEN.read();

            case ScalarUnitRegisterType::c4: {
                SP_STATUS.set(SP_STATUS_RegisterLayout::Field::HALTED, mCPU->getHalt());
                return SP_STATUS.read();
            }

            case ScalarUnitRegisterType::c5:    return SP_STATUS.get(SP_STATUS_RegisterLayout::Field::DMA_FULL).as<BIT>();
            case ScalarUnitRegisterType::c6:    return SP_STATUS.get(SP_STATUS_RegisterLayout::Field::DMA_BUSY).as<BIT>();
            case ScalarUnitRegisterType::c7:    return SP_SEMAPHORE.read();
        }
        return 0;
    }

    void ScalarUnit::setRegister(RegisterIndex reg, U64 value)
    {
        switch (static_cast<ScalarUnitRegisterType>(reg.Value)) {
            case ScalarUnitRegisterType::c0:    SP_DMA_SPADDR.write(value); break;
            case ScalarUnitRegisterType::c1:    SP_DMA_RAMADDR.write(value); break;
            case ScalarUnitRegisterType::c2:    SP_DMA_RDLEN.write(value); break;
            case ScalarUnitRegisterType::c3:    SP_DMA_WRLEN.write(value); break;

            case ScalarUnitRegisterType::c4: {
                SP_STATUS_Write_Register writeReg;
                writeReg.write(value);

                if (writeReg.get(SP_STATUS_Write_RegisterLayout::Field::CLR_HALT).as<BIT>() == ESX_TRUE) mCPU->setHalt(ESX_FALSE);
                if (writeReg.get(SP_STATUS_Write_RegisterLayout::Field::SET_HALT).as<BIT>() == ESX_TRUE) mCPU->setHalt(ESX_TRUE);
                if (writeReg.get(SP_STATUS_Write_RegisterLayout::Field::CLR_BROKE).as<BIT>() == ESX_TRUE) SP_STATUS.set(SP_STATUS_RegisterLayout::Field::BROKE, ESX_FALSE);
                if (writeReg.get(SP_STATUS_Write_RegisterLayout::Field::CLR_SSTEP).as<BIT>() == ESX_TRUE) SP_STATUS.set(SP_STATUS_RegisterLayout::Field::SSTEP, ESX_FALSE);
                if (writeReg.get(SP_STATUS_Write_RegisterLayout::Field::SET_SSTEP).as<BIT>() == ESX_TRUE) SP_STATUS.set(SP_STATUS_RegisterLayout::Field::SSTEP, ESX_TRUE);
                if (writeReg.get(SP_STATUS_Write_RegisterLayout::Field::CLR_INTBREAK).as<BIT>() == ESX_TRUE) SP_STATUS.set(SP_STATUS_RegisterLayout::Field::INTBREAK, ESX_FALSE);
                if (writeReg.get(SP_STATUS_Write_RegisterLayout::Field::SET_INTBREAK).as<BIT>() == ESX_TRUE) SP_STATUS.set(SP_STATUS_RegisterLayout::Field::INTBREAK, ESX_TRUE);

                U16 clrSetSig = writeReg.get(SP_STATUS_Write_RegisterLayout::Field::CLR_SET_SIG).as<U16>();
                U8 sig = SP_STATUS.get(SP_STATUS_RegisterLayout::Field::SIG).as<U8>();

                for (I32 i = 0; i < 8; i++) {
                    if (clrSetSig & (1 << (i * 2))) {
                        sig &= ~(1 << i);
                    }

                    if (clrSetSig & (1 << (i * 2 + 1))) {
                        sig |= (1 << i);
                    }
                }

                SP_STATUS.set(SP_STATUS_RegisterLayout::Field::SIG, sig);
                break;
            }

            case ScalarUnitRegisterType::c7:    SP_SEMAPHORE.write(value); break;
        }
    }

}
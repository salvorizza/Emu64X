#include "ScalarUnit.h"

#include "Core/MIPS/R4000/R4000.h"

#include "Core/RCP/RCP.h"
#include "Core/MIPS/VR4300/VR4300.h"
#include "Core/Scheduler.h"

namespace esx {
    ScalarUnit::ScalarUnit(R4000* cpu, RCP* rcp)
        :   Coprocessor(cpu, 0),
            mRCP(rcp)
    {
        Scheduler::AddSchedulerEventHandler(SchedulerEventType::SPDMADone, [&](SchedulerEvent& ev) {
            U32 NumRows = 0, NumDWords = 0, Length = 0;

            BIT Write = ev.Read<BIT>();
            SP_DMA_SPADDR_Register SP_DMA_SPADDR = ev.Read<SP_DMA_SPADDR_Register>();
            SP_DMA_RAMADDR_Register SP_DMA_RAMADDR = ev.Read<SP_DMA_RAMADDR_Register>();

            U32 RDRAMAddr = SP_DMA_RAMADDR.get(layouts::SP_DMA_RAMADDR_Register::Field::DRAM_ADDR).as<U32>() << 3;
            U32 MEMAddr = 0x04000000 + (SP_DMA_SPADDR.get(layouts::SP_DMA_SPADDR_Register::Field::MEM_BANK).as<U8>() << 12) + (SP_DMA_SPADDR.get(layouts::SP_DMA_SPADDR_Register::Field::MEM_ADDR).as<U32>() << 3);
            if (Write == ESX_FALSE) {
                SP_DMA_RDLEN_Register SP_DMA_RDLEN = ev.Read<SP_DMA_RDLEN_Register>();
                Length = ((SP_DMA_RDLEN.get(layouts::SP_DMA_RDLEN_Register::Field::RDLEN).as<U32>() + 1) << 3);
                NumRows = SP_DMA_RDLEN.get(layouts::SP_DMA_RDLEN_Register::Field::COUNT).as<U16>() + 1;
            } else {
                SP_DMA_WRLEN_Register SP_DMA_WRLEN = ev.Read<SP_DMA_WRLEN_Register>();
                Length = ((SP_DMA_WRLEN.get(layouts::SP_DMA_WRLEN_Register::Field::WRLEN).as<U32>() + 1) << 3);
                NumRows = SP_DMA_WRLEN.get(layouts::SP_DMA_WRLEN_Register::Field::COUNT).as<U16>() + 1;
            }
            NumDWords = (Length / 8) + ((Length % 8) > 0 ? 1 : 0);

            for (I32 row = 0; row < NumRows; row++) {
                for (I32 numDWord = 0; numDWord < NumDWords; numDWord++) {
                    if (Write == ESX_FALSE) {
                        U32 lo = mRCP->SysADLoad(RDRAMAddr + 4, sizeof(U32) * 8);
                        mRCP->store("Root", MEMAddr + 4, lo, 0, sizeof(U32) * 8);

                        U32 hi = mRCP->SysADLoad(RDRAMAddr + 0, sizeof(U32) * 8);
                        mRCP->store("Root", MEMAddr + 0, hi, 0, sizeof(U32) * 8);
                    } else {
                        U32 lo, hi;
                        mRCP->load("Root", MEMAddr + 4, lo, 0, sizeof(U32) * 8);
                        mRCP->SysADStore(RDRAMAddr + 4, sizeof(U32) * 8, lo);

                        mRCP->load("Root", MEMAddr + 0, hi, 0, sizeof(U32) * 8);
                        mRCP->SysADStore(RDRAMAddr + 0, sizeof(U32) * 8, hi);
                    }

                    RDRAMAddr += 8;
                    MEMAddr += 8;
                }
                RDRAMAddr += (SP_DMA_RDLEN.get(layouts::SP_DMA_RDLEN_Register::Field::SKIP).as<U32>() << 3);
            }
            
            if (Write == ESX_FALSE) {
                SP_DMA_RDLEN.set(layouts::SP_DMA_RDLEN_Register::Field::RDLEN, 0xFF8);
            } else {
                SP_DMA_WRLEN.set(layouts::SP_DMA_WRLEN_Register::Field::WRLEN, 0xFF8);
            }

            if (SP_STATUS.get(layouts::SP_STATUS_Register::Field::DMA_FULL).as<BIT>() == ESX_TRUE) {
                SP_STATUS.set(layouts::SP_STATUS_Register::Field::DMA_FULL, ESX_FALSE);
            } else {
                SP_STATUS.set(layouts::SP_STATUS_Register::Field::DMA_BUSY, ESX_FALSE);
            }
        });
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
        SP_STATUS.set(layouts::SP_STATUS_Register::Field::BROKE, ESX_TRUE);
        mCPU->setHalt(ESX_TRUE);

        if (SP_STATUS.get(layouts::SP_STATUS_Register::Field::INTBREAK).as<BIT>() == ESX_TRUE) {
            mRCP->setInterrupt(InterruptType::SP, ESX_FALSE, ESX_TRUE, 0);
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
                SP_STATUS.set(layouts::SP_STATUS_Register::Field::HALTED, mCPU->getHalt());
                return SP_STATUS.read();
            }

            case ScalarUnitRegisterType::c5:    return SP_STATUS.get(layouts::SP_STATUS_Register::Field::DMA_FULL).as<BIT>();
            case ScalarUnitRegisterType::c6:    return SP_STATUS.get(layouts::SP_STATUS_Register::Field::DMA_BUSY).as<BIT>();
            case ScalarUnitRegisterType::c7:    return SP_SEMAPHORE.read();
        }
        return 0;
    }

    void ScalarUnit::setRegister(RegisterIndex reg, U64 value)
    {
        switch (static_cast<ScalarUnitRegisterType>(reg.Value)) {
            case ScalarUnitRegisterType::c0:    SP_DMA_SPADDR.write(value); break;
            case ScalarUnitRegisterType::c1:    SP_DMA_RAMADDR.write(value); break;
            case ScalarUnitRegisterType::c2: {
                SP_DMA_RDLEN.write(value);

                if (SP_STATUS.get(layouts::SP_STATUS_Register::Field::DMA_FULL).as<BIT>() == ESX_FALSE) {
                    BIT DMAAlreadyUp = SP_STATUS.get(layouts::SP_STATUS_Register::Field::DMA_BUSY).as<BIT>();
                    SP_STATUS.set(layouts::SP_STATUS_Register::Field::DMA_FULL, DMAAlreadyUp);
                    SP_STATUS.set(layouts::SP_STATUS_Register::Field::DMA_BUSY, ESX_TRUE);

                    U64 cpuClocks = mRCP->getBus("Root")->getDevice<VR4300>("VR4300")->getClocks();

                    U64 TargetClock = DMAAlreadyUp ? Scheduler::NextEventOfType(SchedulerEventType::SPDMADone).value()->ClockTarget : cpuClocks;
                    U64 ClockToAdd = (((SP_DMA_RDLEN.get(layouts::SP_DMA_RDLEN_Register::Field::COUNT).as<U8>() + 1) * ((SP_DMA_RDLEN.get(layouts::SP_DMA_RDLEN_Register::Field::RDLEN).as<U32>() + 1) << 3)) * 37) / 10;

                    SchedulerEvent dmaDoneEvent = {
                            .Type = SchedulerEventType::SPDMADone,
                            .ClockStart = cpuClocks,
                            .ClockTarget = TargetClock + ClockToAdd
                    };
                    dmaDoneEvent.Write(ESX_FALSE);
                    dmaDoneEvent.Write(SP_DMA_SPADDR);
                    dmaDoneEvent.Write(SP_DMA_RAMADDR);
                    dmaDoneEvent.Write(SP_DMA_RDLEN);


                    Scheduler::ScheduleEvent(dmaDoneEvent);
                }
                break;
            }

            case ScalarUnitRegisterType::c3: {
                SP_DMA_WRLEN.write(value);

                if (SP_STATUS.get(layouts::SP_STATUS_Register::Field::DMA_FULL).as<BIT>() == ESX_FALSE) {
                    BIT DMAAlreadyUp = SP_STATUS.get(layouts::SP_STATUS_Register::Field::DMA_BUSY).as<BIT>();
                    SP_STATUS.set(layouts::SP_STATUS_Register::Field::DMA_FULL, DMAAlreadyUp);
                    SP_STATUS.set(layouts::SP_STATUS_Register::Field::DMA_BUSY, ESX_TRUE);

                    U64 cpuClocks = mRCP->getBus("Root")->getDevice<VR4300>("VR4300")->getClocks();

                    U64 TargetClock = DMAAlreadyUp ? Scheduler::NextEventOfType(SchedulerEventType::SPDMADone).value()->ClockTarget : cpuClocks;
                    U64 ClockToAdd = (((SP_DMA_WRLEN.get(layouts::SP_DMA_WRLEN_Register::Field::COUNT).as<U8>() + 1) * ((SP_DMA_WRLEN.get(layouts::SP_DMA_WRLEN_Register::Field::WRLEN).as<U32>() + 1) << 3)) * 37) / 10;

                    SchedulerEvent dmaDoneEvent = {
                            .Type = SchedulerEventType::SPDMADone,
                            .ClockStart = cpuClocks,
                            .ClockTarget = TargetClock + ClockToAdd
                    };
                    dmaDoneEvent.Write(ESX_FALSE);
                    dmaDoneEvent.Write(SP_DMA_SPADDR);
                    dmaDoneEvent.Write(SP_DMA_RAMADDR);
                    dmaDoneEvent.Write(SP_DMA_WRLEN);

                    Scheduler::ScheduleEvent(dmaDoneEvent);
                }
                break;
            }

            case ScalarUnitRegisterType::c4: {
                SP_STATUS_Write_Register writeReg;
                writeReg.write(value);

                if (writeReg.get(layouts::SP_STATUS_Write_Register::Field::CLR_HALT).as<BIT>() == ESX_TRUE) 
                    mCPU->setHalt(ESX_FALSE);

                if (writeReg.get(layouts::SP_STATUS_Write_Register::Field::SET_HALT).as<BIT>() == ESX_TRUE) 
                    mCPU->setHalt(ESX_TRUE);

                if (writeReg.get(layouts::SP_STATUS_Write_Register::Field::CLR_BROKE).as<BIT>() == ESX_TRUE) 
                    SP_STATUS.set(layouts::SP_STATUS_Register::Field::BROKE, ESX_FALSE);

                if (writeReg.get(layouts::SP_STATUS_Write_Register::Field::CLR_SSTEP).as<BIT>() == ESX_TRUE) 
                    SP_STATUS.set(layouts::SP_STATUS_Register::Field::SSTEP, ESX_FALSE);

                if (writeReg.get(layouts::SP_STATUS_Write_Register::Field::SET_INTR).as<BIT>() == ESX_TRUE)
                    mRCP->setInterrupt(InterruptType::SP, ESX_FALSE, ESX_TRUE, 0);

                if (writeReg.get(layouts::SP_STATUS_Write_Register::Field::CLR_INTR).as<BIT>() == ESX_TRUE) 
                    mRCP->clearInterrupt(InterruptType::SP);

                if (writeReg.get(layouts::SP_STATUS_Write_Register::Field::SET_SSTEP).as<BIT>() == ESX_TRUE) 
                    SP_STATUS.set(layouts::SP_STATUS_Register::Field::SSTEP, ESX_TRUE);

                if (writeReg.get(layouts::SP_STATUS_Write_Register::Field::CLR_INTBREAK).as<BIT>() == ESX_TRUE)
                    SP_STATUS.set(layouts::SP_STATUS_Register::Field::INTBREAK, ESX_FALSE);

                if (writeReg.get(layouts::SP_STATUS_Write_Register::Field::SET_INTBREAK).as<BIT>() == ESX_TRUE)
                    SP_STATUS.set(layouts::SP_STATUS_Register::Field::INTBREAK, ESX_TRUE);

                U16 clrSetSig = writeReg.get(layouts::SP_STATUS_Write_Register::Field::CLR_SET_SIG).as<U16>();
                U8 sig = SP_STATUS.get(layouts::SP_STATUS_Register::Field::SIG).as<U8>();

                for (I32 i = 0; i < 8; i++) {
                    if (clrSetSig & (1 << (i * 2))) {
                        sig &= ~(1 << i);
                    }

                    if (clrSetSig & (1 << (i * 2 + 1))) {
                        sig |= (1 << i);
                    }
                }

                SP_STATUS.set(layouts::SP_STATUS_Register::Field::SIG, sig);
                break;
            }

            case ScalarUnitRegisterType::c7:    SP_SEMAPHORE.write(value); break;
            default: ESX_CORE_LOG_WARNING("SU {} not implemented", reg.Value);
              
        }
    }

}
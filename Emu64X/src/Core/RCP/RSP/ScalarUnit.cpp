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
            U32 MEMBase = 0x04000000 + (SP_DMA_SPADDR.get(layouts::SP_DMA_SPADDR_Register::Field::MEM_BANK).as<U8>() << 12);
            U32 MEMAddr = MEMBase + (SP_DMA_SPADDR.get(layouts::SP_DMA_SPADDR_Register::Field::MEM_ADDR).as<U32>() << 3);
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

                    MEMAddr &= (MEMBase | 0xFFF);
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
    
        Scheduler::AddSchedulerEventHandler(SchedulerEventType::DPDMADone, [&](SchedulerEvent& ev) {
            DPC_START_Register DPC_START = ev.Read<DPC_START_Register>();
            DPC_END_Register DPC_END = ev.Read<DPC_END_Register>();

            DPC_CURRENT.set(layouts::DPC_CURRENT_Register::Field::CURRENT, DPC_END.get(layouts::DPC_END_Register::Field::END).as<U32>());
            DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::DMA_BUSY, ESX_FALSE);
            if (mPendingTransfer) {
                U64 cpuClocks = mRCP->getBus("Root")->getDevice<VR4300>("VR4300")->getClocks();

                SchedulerEvent dmaDoneEvent = {
                    .Type = SchedulerEventType::DPDMADone,
                    .ClockStart = cpuClocks,
                    .ClockTarget = cpuClocks + mPendingTransfer->Clocks()
                };
                dmaDoneEvent.Write(mPendingTransfer->Start);
                dmaDoneEvent.Write(mPendingTransfer->End);

                DPC_CURRENT.set(layouts::DPC_CURRENT_Register::Field::CURRENT, mPendingTransfer->Start.get(layouts::DPC_START_Register::Field::START).as<U32>() & ~0x7);
                DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::END_PENDING, ESX_FALSE);
                if (mPendingTransfer->Clocks() > 0) {
                    Scheduler::ScheduleEvent(dmaDoneEvent);
                    DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::PIPE_BUSY, ESX_TRUE);
                    DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::DMA_BUSY, ESX_TRUE);
                }
                DoTransferRDPCommands(mPendingTransfer->Start.read(), mPendingTransfer->End.read());

                mPendingTransfer = {};
            }
        });


    }

    void ScalarUnit::clock(U64 clocks)
    {
    }

    void ScalarUnit::CF()
    {
        ESX_CORE_LOG_WARNING("{} - Not implemented yet", __FUNCTION__);
    }

    void ScalarUnit::CT()
    {
        ESX_CORE_LOG_WARNING("{} - Not implemented yet", __FUNCTION__);
    }

    void ScalarUnit::CO()
    {
        
    }

    void ScalarUnit::unusable()
    {
        ESX_CORE_LOG_WARNING("{}", __FUNCTION__);
    }

    void ScalarUnit::reserved()
    {
        ESX_CORE_LOG_WARNING("{}", __FUNCTION__);
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
            case ScalarUnitRegisterType::c8:    return DPC_START.read();
            case ScalarUnitRegisterType::c9:    return DPC_END.read();
            case ScalarUnitRegisterType::c10: {
                auto TransferEvent = Scheduler::NextEventOfType(SchedulerEventType::DPDMADone);
                if (TransferEvent) {
                    U64 NumBytes = ((mRCP->getBus("Root")->getDevice<VR4300>("VR4300")->getClocks() - TransferEvent.value()->ClockStart) / 10) * 37;
                    DPC_CURRENT.set(layouts::DPC_CURRENT_Register::Field::CURRENT, DPC_CURRENT.get(layouts::DPC_CURRENT_Register::Field::CURRENT).as<U32>() + (NumBytes / 8));
                }

                return DPC_CURRENT.read();
            }
            case ScalarUnitRegisterType::c11:   return DPC_STATUS.read();

            default: {
                ESX_CORE_LOG_WARNING("{} - SU {} not implemented", __FUNCTION__, reg.Value);
            }
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

                    U32 Length = ((SP_DMA_RDLEN.get(layouts::SP_DMA_RDLEN_Register::Field::RDLEN).as<U32>() + 1) << 3);
                    U16 NumRows = SP_DMA_RDLEN.get(layouts::SP_DMA_RDLEN_Register::Field::COUNT).as<U16>() + 1;
                    U32 NumDWords = (Length / 8) + ((Length % 8) > 0 ? 1 : 0);
                    U32 NumBytes = NumRows * NumDWords * 8;
                    U64 TargetClock = DMAAlreadyUp ? Scheduler::NextEventOfType(SchedulerEventType::SPDMADone).value()->ClockTarget : cpuClocks;
                    U64 ClockToAdd = (NumBytes * 10) / (37 * 2);

                    SchedulerEvent dmaDoneEvent = {
                            .Type = SchedulerEventType::SPDMADone,
                            .ClockStart = cpuClocks,
                            .ClockTarget = TargetClock + ClockToAdd,
                            .Priority = DMAAlreadyUp ? 1 : 0
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

                    U32 Length = ((SP_DMA_RDLEN.get(layouts::SP_DMA_RDLEN_Register::Field::RDLEN).as<U32>() + 1) << 3);
                    U16 NumRows = SP_DMA_RDLEN.get(layouts::SP_DMA_RDLEN_Register::Field::COUNT).as<U16>() + 1;
                    U32 NumDWords = (Length / 8) + ((Length % 8) > 0 ? 1 : 0);
                    U32 NumBytes = NumRows * NumDWords * 8;
                    U64 TargetClock = DMAAlreadyUp ? Scheduler::NextEventOfType(SchedulerEventType::SPDMADone).value()->ClockTarget : cpuClocks;
                    U64 ClockToAdd = (NumBytes * 10) / (37 * 2);

                    SchedulerEvent dmaDoneEvent = {
                            .Type = SchedulerEventType::SPDMADone,
                            .ClockStart = cpuClocks,
                            .ClockTarget = TargetClock + ClockToAdd,
                            .Priority = DMAAlreadyUp ? 1 : 0
                    };
                    dmaDoneEvent.Write(ESX_TRUE);
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

            case ScalarUnitRegisterType::c8: {
                DPC_START.write(value);
                DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::START_PENDING, ESX_TRUE);
                break;
            }

            case ScalarUnitRegisterType::c9: {
                DPC_END_Register OldDPC_END = DPC_END;
                DPC_END.write(value);


                auto TransferEvent = Scheduler::NextEventOfType(SchedulerEventType::DPDMADone);
                if (DPC_STATUS.get(layouts::DPC_STATUS_Register::Field::START_PENDING).as<BIT>() == ESX_FALSE) {
                    DPC_START_Register NewStart;
                    NewStart.write(TransferEvent ? DPC_START.read() : OldDPC_END.read());

                    U64 TransferLength = (DPC_END.get(layouts::DPC_END_Register::Field::END).as<U32>() & ~0x7) - (NewStart.get(layouts::DPC_START_Register::Field::START).as<U32>() & ~0x7);
                    U64 ClockStart = TransferEvent ? TransferEvent.value()->ClockStart : mRCP->getBus("Root")->getDevice<VR4300>("VR4300")->getClocks();
                    U64 ClockEnd = (TransferEvent ? TransferEvent.value()->ClockTarget : ClockStart) + (TransferLength * 10) / (37 * 2);

                    SchedulerEvent dmaDoneEvent = {
                            .Type = SchedulerEventType::DPDMADone,
                            .ClockStart = ClockStart,
                            .ClockTarget = ClockEnd,
                            .Priority = 0
                    };
                    dmaDoneEvent.Write(NewStart);
                    dmaDoneEvent.Write(DPC_END);

                    Scheduler::UnScheduleAllEvents(SchedulerEventType::DPDMADone);

                    if (TransferLength > 0) {
                        Scheduler::ScheduleEvent(dmaDoneEvent);
                        DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::PIPE_BUSY, ESX_TRUE);
                        DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::DMA_BUSY, ESX_TRUE);
                    }

                    DoTransferRDPCommands(NewStart.read(), DPC_END.read());
                }
                else {
                    if (TransferEvent || DPC_STATUS.get(layouts::DPC_STATUS_Register::Field::END_PENDING).as<BIT>() == ESX_TRUE) {
                        DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::END_PENDING, ESX_TRUE);

                        if (mPendingTransfer) {
                            mPendingTransfer->Start = DPC_START;
                            mPendingTransfer->End = DPC_END;
                        } else {
                            mPendingTransfer = TransferData{
                                .Start = DPC_START,
                                .End = DPC_END
                            };
                        }
                    }
                    else {
                        U64 TransferLength = (DPC_END.get(layouts::DPC_END_Register::Field::END).as<U32>() & ~0x7) - (DPC_START.get(layouts::DPC_START_Register::Field::START).as<U32>() & ~0x7);

                        DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::START_PENDING, ESX_FALSE);
                            
                        U64 ClockStart = mRCP->getBus("Root")->getDevice<VR4300>("VR4300")->getClocks();
                        U64 ClockEnd = ClockStart + (TransferLength * 10) / (37 * 2);

                        SchedulerEvent dmaDoneEvent = {
                                .Type = SchedulerEventType::DPDMADone,
                                .ClockStart = ClockStart,
                                .ClockTarget = ClockEnd
                        };
                        dmaDoneEvent.Write(DPC_START.read());
                        dmaDoneEvent.Write(DPC_END);

                        DPC_CURRENT.set(layouts::DPC_CURRENT_Register::Field::CURRENT, DPC_START.get(layouts::DPC_START_Register::Field::START).as<U32>() & ~0x7);
                        if (TransferLength > 0) {
                            Scheduler::ScheduleEvent(dmaDoneEvent);
                            DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::PIPE_BUSY, ESX_TRUE);
                            DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::DMA_BUSY, ESX_TRUE);
                        }

                        DoTransferRDPCommands(DPC_START.read(), DPC_END.read());
                    }
                }
                break;
            }

            case ScalarUnitRegisterType::c11: {
                DPC_STATUS_Write_Register writeReg;
                writeReg.write(value);

                if(writeReg.get(layouts::DPC_STATUS_Write_Register::Field::CLR_XBUS).as<BIT>() == ESX_TRUE) {
                    DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::XBUS, ESX_FALSE);
                }

                if (writeReg.get(layouts::DPC_STATUS_Write_Register::Field::SET_XBUS).as<BIT>() == ESX_TRUE) {
                    DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::XBUS, ESX_TRUE);
                    ESX_CORE_LOG_WARNING("SET_XBUS not implemented yet");
                }

                if (writeReg.get(layouts::DPC_STATUS_Write_Register::Field::CLR_FREEZE).as<BIT>() == ESX_TRUE) {
                    DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::FREEZE, ESX_FALSE);
                }

                if (writeReg.get(layouts::DPC_STATUS_Write_Register::Field::SET_FREEZE).as<BIT>() == ESX_TRUE) {
                    DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::FREEZE, ESX_TRUE);
                    ESX_CORE_LOG_WARNING("SET_FREEZE not implemented yet");
                }

                if (writeReg.get(layouts::DPC_STATUS_Write_Register::Field::CLR_FLUSH).as<BIT>() == ESX_TRUE) {
                    DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::FLUSH, ESX_FALSE);
                }

                if (writeReg.get(layouts::DPC_STATUS_Write_Register::Field::SET_FLUSH).as<BIT>() == ESX_TRUE) {
                    DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::FLUSH, ESX_TRUE);
                    ESX_CORE_LOG_WARNING("SET_FLUSH not implemented yet");
                }

                if (writeReg.get(layouts::DPC_STATUS_Write_Register::Field::CLR_TMEM_BUSY).as<BIT>() == ESX_TRUE) {
                    ESX_CORE_LOG_WARNING("Cear DPC_TMEM_BUSY to 0 not implemented yet");
                }

                if (writeReg.get(layouts::DPC_STATUS_Write_Register::Field::CLR_PIPE_BUSY).as<BIT>() == ESX_TRUE) {
                    ESX_CORE_LOG_WARNING("Cear DPC_PIPE_BUSY to 0 not implemented yet");
                }

                if (writeReg.get(layouts::DPC_STATUS_Write_Register::Field::CLR_BUFFER_BUSY).as<BIT>() == ESX_TRUE) {
                    ESX_CORE_LOG_WARNING("Cear DPC_BUSY to 0 not implemented yet");
                }

                if (writeReg.get(layouts::DPC_STATUS_Write_Register::Field::CLR_CLOCK).as<BIT>() == ESX_TRUE) {
                    ESX_CORE_LOG_WARNING("Cear DPC_CLOCK to 0 not implemented yet");
                }
                break;
            }

            default: { 
                ESX_CORE_LOG_WARNING("{} - SU {} not implemented {:08x}h", __FUNCTION__, reg.Value, value); 
            }
        }
    }

    constexpr Array<StringView, 64> sCommands = {
        // 0x00
        "No Operation", "No Operation", "No Operation", "No Operation",
        "No Operation", "No Operation", "No Operation", "No Operation",

        // 0x08
        "Fill Triangle", "Fill Triangle (Z)", "Fill Triangle (T)", "Fill Triangle (TZ)",
        "Fill Triangle (S)", "Fill Triangle (SZ)", "Fill Triangle (ST)", "Fill Triangle (STZ)",

        // 0x10
        "No Operation", "No Operation", "No Operation", "No Operation",
        "No Operation", "No Operation", "No Operation", "No Operation",

        // 0x18
        "No Operation", "No Operation", "No Operation", "No Operation",
        "No Operation", "No Operation", "No Operation", "No Operation",

        // 0x20
        "No Operation", "No Operation", "No Operation", "No Operation",
        "Texture Rectangle", "Texture Rectangle Flip", "Sync Load", "Sync Pipe",

        // 0x28
        "Sync Tile", "Sync Full", "Set Key GB", "Set Key R",
        "Set Convert", "Set Scissor", "Set Primitive Depth", "Set Other Modes",

        // 0x30
        "Load TLUT", "No Operation", "Set Tile Size", "Load Block",
        "Load Tile", "Set Tile", "Fill Rectangle", "Set Fill Color",

        // 0x38
        "Set Fog Color", "Set Blend Color", "Set Primitive Color", "Set Environment Color",
        "Set Combine Mode", "Set Texture Image", "Set Depth Image", "Set Color Image"
    };

    void ScalarUnit::DoTransferRDPCommands(U32 start, U32 end)
    {
        start &= ~0x7;
        end &= ~0x7;

        U32 RDRAMAddr = start;
        U64 TransferLength = end - start;
        U64 NumDWords = TransferLength / 8;

        for (I32 numDWord = 0; numDWord < NumDWords; numDWord++) {
            U32 lo = mRCP->SysADLoad(RDRAMAddr + 4, sizeof(U32) * 8);
            U32 hi = mRCP->SysADLoad(RDRAMAddr + 0, sizeof(U32) * 8);
            U64 command = (static_cast<U64>(hi) << 32) | lo;

            ESX_CORE_LOG_TRACE("RDP Full Command {:08x}h - Command => {}", command, sCommands[(command >> 56) & 0x3F]);

            if (((command >> 56) & 0x3F) == 0x29) {
                mRCP->setInterrupt(InterruptType::DP, ESX_FALSE, ESX_TRUE, 0);
                DPC_STATUS.set(layouts::DPC_STATUS_Register::Field::PIPE_BUSY, ESX_FALSE);
            }

            RDRAMAddr += 8;
        }
    }

}
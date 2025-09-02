#include "SystemControlCoprocessor.h"

#include "VR4300.h"

namespace esx {
    SystemControlCoprocessor::SystemControlCoprocessor(VR4300* cpu)
        : Coprocessor(cpu, 0)
    {
        mRandomRegister.set(RandomRegisterLayout::Field::Random, 31);
    }

    void SystemControlCoprocessor::clock(U64 clocks)
    {
        if (mCountRegister.read() == mCompareRegister.read()) {
            generateInterrupt(Interrupt::IP7);
        }
        mCountRegister.write(mCountRegister.read() + 2);

        U8 random = mRandomRegister.get(RandomRegisterLayout::Field::Random).as<U8>();
        U8 wired = mWiredRegister.get(WiredRegisterLayout::Field::Wired).as<U8>();
        random--;
        if (random < wired) random = wired;
        mRandomRegister.set(RandomRegisterLayout::Field::Random, random);

        Coprocessor::clock(clocks);

        handleInterrupts();
    }

    void SystemControlCoprocessor::CO()
    {
        switch (mCPU->mCurrentInstruction.CoprocessorFunction()) {
            case 1: {
                TLBR();
                break;
            }

            case 2: {
                TLBWI();
                break;
            }

            case 6: {
                TLBWR();
                break;
            }

            case 8: {
                TLBP();
                break;
            }

            case 16: {
                raiseException(ExceptionType::ReservedInstruction);
                break;
            }

            case 24: {
                ERET();
                break;
            }
        }
    }

    void SystemControlCoprocessor::TLBR()
    {
        if (!isCoprocessorUsable(0)) {
            mCPU->raiseException(ExceptionType::CoprocessorUnusable);
            return;
        }

        U8 index = mIndexRegister.get(IndexRegisterLayout::Field::Index).as<U8>();

        TLBEntry& entry = mTLB[index];

        mPageMaskRegister.write(entry.PageMask.read());

        mEntryHiRegister.write(entry.EntryHi.read() & ~(entry.PageMask.read()));

        mEntryLo1Register.write(entry.EntryLo1.read());
        mEntryLo1Register.set(EntryLoRegisterLayout::Field::G, entry.EntryHi.get(EntryHiRegisterLayout::Field::G).as<BIT>());

        mEntryLo0Register.write(entry.EntryLo0.read());
        mEntryLo0Register.set(EntryLoRegisterLayout::Field::G, entry.EntryHi.get(EntryHiRegisterLayout::Field::G).as<BIT>());
    }

    void SystemControlCoprocessor::TLBWI()
    {
        if (!isCoprocessorUsable(0)) {
            mCPU->raiseException(ExceptionType::CoprocessorUnusable);
            return;
        }

        U8 index = mIndexRegister.get(IndexRegisterLayout::Field::Index).as<U8>();

        TLBEntry& entry = mTLB[index];

        entry.PageMask.write(mPageMaskRegister.read());

        entry.EntryHi.write(mEntryHiRegister.read() & ~(mPageMaskRegister.read()));
        entry.EntryHi.set(EntryHiRegisterLayout::Field::G, mEntryLo0Register.get(EntryLoRegisterLayout::Field::G).as<BIT>() & mEntryLo1Register.get(EntryLoRegisterLayout::Field::G).as<BIT>());

        entry.EntryLo1.write(mEntryLo1Register.read());
        
        entry.EntryLo0.write(mEntryLo0Register.read());
    }

    void SystemControlCoprocessor::TLBWR()
    {
        if (!isCoprocessorUsable(0)) {
            mCPU->raiseException(ExceptionType::CoprocessorUnusable);
            return;
        }

        U8 index = mRandomRegister.get(RandomRegisterLayout::Field::Random).as<U8>();

        TLBEntry& entry = mTLB[index];

        entry.PageMask.write(mPageMaskRegister.read());

        entry.EntryHi.write(mEntryHiRegister.read() & ~(mPageMaskRegister.read()));
        entry.EntryHi.set(EntryHiRegisterLayout::Field::G, mEntryLo0Register.get(EntryLoRegisterLayout::Field::G).as<BIT>() & mEntryLo1Register.get(EntryLoRegisterLayout::Field::G).as<BIT>());

        entry.EntryLo1.write(mEntryLo1Register.read());

        entry.EntryLo0.write(mEntryLo0Register.read());
    }

    void SystemControlCoprocessor::TLBP()
    {
        if (!isCoprocessorUsable(0)) {
            mCPU->raiseException(ExceptionType::CoprocessorUnusable);
            return;
        }

        mIndexRegister.set(IndexRegisterLayout::Field::Probe, ESX_TRUE);
        for (U8 i = 0; i < mTLB.size(); i++) {
            TLBEntry& entry = mTLB[i];

            if (
                (entry.EntryHi.get(EntryHiRegisterLayout::Field::VPN2).as<U32>() == mEntryHiRegister.get(EntryHiRegisterLayout::Field::VPN2).as<U32>()) &&
                (entry.EntryHi.get(EntryHiRegisterLayout::Field::G).as<BIT>() == ESX_TRUE || (entry.EntryHi.get(EntryHiRegisterLayout::Field::ASID).as<U32>() == mEntryHiRegister.get(EntryHiRegisterLayout::Field::ASID).as<U32>()))
                ) {
                mIndexRegister.set(IndexRegisterLayout::Field::Index, i);
                mIndexRegister.set(IndexRegisterLayout::Field::Probe, ESX_FALSE);
            }
        }
    }

    void SystemControlCoprocessor::ERET()
    {
        U64 PC = 0;
        if (mStatusRegister.get(StatusRegisterLayout::Field::ERL).as<BIT>() == ESX_TRUE) {
            PC = mErrorEPCRegister.get(ErrorEPCRegisterLayout::Field::Value).as<U64>();
            mStatusRegister.set(StatusRegisterLayout::Field::ERL, ESX_FALSE);
        } else {
            PC = mEPCRegister.get(EPCRegisterLayout::Field::Value).as<U64>();
            mStatusRegister.set(StatusRegisterLayout::Field::EXL, ESX_FALSE);
        }

        mCPU->mPC = PC;
        mCPU->mNextPC = PC + 4;

        mCPU->mLLBit = ESX_FALSE;
    }

    void SystemControlCoprocessor::handleInterrupts()
    {
        if (areInterruptsPending() && areInterruptsEnabled()) {
            raiseException(ExceptionType::Interrupt);
        }
    }

    void SystemControlCoprocessor::raiseException(ExceptionType type)
    {
        /*Set FP Control Status Register
        EnHi < -VPN2, ASID
        X / Context < -VPN2
        Set Cause Register
        EXcCode, CE
        BadVAddr Register Setting
            Comments
            ; FP Control/Status Register are 
            only set if the respective exception 
            occurs.
            EnHi, X/Context are set only for 
            TLB-Invalid, Modification & Miss 
            exceptions. It is not set by bus 
            error exceptions, however.
        */

        mCauseRegister.set(CauseRegisterLayout::Field::ExcCode, (U8)type);
        if(type == ExceptionType::CoprocessorUnusable) mCauseRegister.set(CauseRegisterLayout::Field::CE, 0);

        if (mStatusRegister.get(StatusRegisterLayout::Field::EXL).as<BIT>() == ESX_FALSE) {
            if (mCPU->mBranchSlot == ESX_FALSE) {
                mCauseRegister.set(CauseRegisterLayout::Field::BD, ESX_FALSE);
                mEPCRegister.set(EPCRegisterLayout::Field::Value, mCPU->mCurrentPC);
            } else {
                mCauseRegister.set(CauseRegisterLayout::Field::BD, ESX_TRUE);
                mEPCRegister.set(EPCRegisterLayout::Field::Value, mCPU->mCurrentPC - 4);
            }
        }

        mStatusRegister.set(StatusRegisterLayout::Field::EXL, ESX_TRUE);

        if (mStatusRegister.get(StatusRegisterLayout::Field::BEV).as<BIT>() == ESX_TRUE) {
            mCPU->mPC = 0xBFC00200 + 180;
        } else {
            mCPU->mPC = 0x80000000 + 180;
        }

        mCPU->mNextPC = mCPU->mPC + 4;
    }

    U32 SystemControlCoprocessor::AddressTranslation(U32 virtualAddress)
    {
        U32 physicalAddress = 0;

        if (virtualAddress >= 0x80000000 && virtualAddress <= 0xBFFFFFFF) {
            //KSEG0,KSEG1 directly mapped
            physicalAddress = virtualAddress & 0x1FFFFFFF;
        }

        return physicalAddress;
    }

    U64 SystemControlCoprocessor::getRegister(RegisterIndex reg) const
    {
        switch (static_cast<SystemControlRegisterType>(reg.Value)) {
            case SystemControlRegisterType::Index:    return mIndexRegister.read();
            case SystemControlRegisterType::Random:   return mRandomRegister.read();
            case SystemControlRegisterType::EntryLo0: return mEntryLo0Register.read();
            case SystemControlRegisterType::EntryLo1: return mEntryLo1Register.read();
            case SystemControlRegisterType::Context:  return mContextRegister.read();
            case SystemControlRegisterType::PageMask: return mPageMaskRegister.read();
            case SystemControlRegisterType::Wired:    return mWiredRegister.read();
            case SystemControlRegisterType::BadVAddr: return mBadVAddrRegister.read();
            case SystemControlRegisterType::Count:    return mCountRegister.read();
            case SystemControlRegisterType::EntryHi:  return mEntryHiRegister.read();
            case SystemControlRegisterType::Compare:  return mCompareRegister.read();
            case SystemControlRegisterType::Status:   return mStatusRegister.read();
            case SystemControlRegisterType::Cause:    return mCauseRegister.read();
            case SystemControlRegisterType::EPC:      return mEPCRegister.read();
            case SystemControlRegisterType::PRId:     return mPRIdRegister.read();
            case SystemControlRegisterType::Config:   return mConfigRegister.read();
            case SystemControlRegisterType::LLAddr:   return mLLAddrRegister.read();
            case SystemControlRegisterType::WatchLo:  return mWatchLoRegister.read();
            case SystemControlRegisterType::WatchHi:  return mWatchHiRegister.read();
            case SystemControlRegisterType::XContext: return mXContextRegister.read();
            case SystemControlRegisterType::PErr:     return mPErrRegister.read();
            case SystemControlRegisterType::CacheErr: return mCacheErrRegister.read();
            case SystemControlRegisterType::TagLo:    return mTagLoRegister.read();
            case SystemControlRegisterType::TagHi:    return mTagHiRegister.read();
            case SystemControlRegisterType::ErrorEPC: return mErrorEPCRegister.read();
        }
        return 0;
    }

    void SystemControlCoprocessor::setRegister(RegisterIndex reg, U64 value)
    {

        switch (static_cast<SystemControlRegisterType>(reg.Value)) {
            case SystemControlRegisterType::Index:    mIndexRegister.write(value); break;
            case SystemControlRegisterType::EntryLo0: mEntryLo0Register.write(value); break;
            case SystemControlRegisterType::EntryLo1: mEntryLo1Register.write(value); break;
            case SystemControlRegisterType::Context:  mContextRegister.write(value); break;
            case SystemControlRegisterType::PageMask: mPageMaskRegister.write(value); break;

            case SystemControlRegisterType::Wired: {
                mWiredRegister.write(value); 
                mRandomRegister.set(RandomRegisterLayout::Field::Random, 31);
                break;
            }

            case SystemControlRegisterType::Count:    mCountRegister.write(value); break;
            case SystemControlRegisterType::EntryHi:  mEntryHiRegister.write(value); break;

            case SystemControlRegisterType::Compare: {
                mCompareRegister.write(value); 
                clearInterrupt(Interrupt::IP7);
                break;
            }

            case SystemControlRegisterType::Status:   mStatusRegister.write(value); break;
            case SystemControlRegisterType::Cause:    mCauseRegister.write(value); break;
            case SystemControlRegisterType::EPC:      mEPCRegister.write(value); break;
            case SystemControlRegisterType::Config:   mConfigRegister.write(value); break;
            case SystemControlRegisterType::LLAddr:   mLLAddrRegister.write(value); break;
            case SystemControlRegisterType::WatchLo:  mWatchLoRegister.write(value); break;
            case SystemControlRegisterType::WatchHi:  mWatchHiRegister.write(value); break;
            case SystemControlRegisterType::XContext: mXContextRegister.write(value); break;
            case SystemControlRegisterType::PErr:     mPErrRegister.write(value); break;
            case SystemControlRegisterType::TagLo:    mTagLoRegister.write(value); break;
            case SystemControlRegisterType::TagHi:    mTagHiRegister.write(value); break;
            case SystemControlRegisterType::ErrorEPC: mErrorEPCRegister.write(value); break;
        }
    }

    void SystemControlCoprocessor::clearInterrupt(Interrupt interrupt)
    {
        mCauseRegister.set(CauseRegisterLayout::Field::IP, mCauseRegister.get(CauseRegisterLayout::Field::IP).as<U8>() & (~static_cast<U8>(interrupt)));
    }

    void SystemControlCoprocessor::generateInterrupt(Interrupt interrupt)
    {
        mCauseRegister.set(CauseRegisterLayout::Field::IP, mCauseRegister.get(CauseRegisterLayout::Field::IP).as<U8>() | static_cast<U8>(interrupt));
    }

    OperatingMode SystemControlCoprocessor::getCurrentOperatingMode() const
    {
        if (mStatusRegister.get(StatusRegisterLayout::Field::EXL).as<BIT>() == ESX_TRUE || mStatusRegister.get(StatusRegisterLayout::Field::ERL).as<BIT>() == ESX_TRUE) {
            return OperatingMode::Kernel;
        }

        return mStatusRegister.get(StatusRegisterLayout::Field::KSU).as<OperatingMode>();
    }

    BIT SystemControlCoprocessor::is64BitMode() const
    {
        switch (getCurrentOperatingMode()) {
            case OperatingMode::Kernel: return mStatusRegister.get(StatusRegisterLayout::Field::KX).as<BIT>();
            case OperatingMode::Supervisor: return mStatusRegister.get(StatusRegisterLayout::Field::SX).as<BIT>();
            case OperatingMode::User: return mStatusRegister.get(StatusRegisterLayout::Field::UX).as<BIT>();
        }
    }

    BIT SystemControlCoprocessor::isCoprocessorUsable(U8 copNumber) const
    {
        if (copNumber == 0 && getCurrentOperatingMode() == OperatingMode::Kernel)
            return ESX_TRUE;

        return mStatusRegister.get(StatusRegisterLayout::Field::CU).as<U8>() & (1 << copNumber);
    }

    BIT SystemControlCoprocessor::isReserved64BitInstruction() const
    {
        switch (getCurrentOperatingMode()) {
            case OperatingMode::Kernel: return ESX_FALSE;
            case OperatingMode::Supervisor: return mStatusRegister.get(StatusRegisterLayout::Field::SX).as<BIT>() == ESX_FALSE;
            case OperatingMode::User: return mStatusRegister.get(StatusRegisterLayout::Field::UX).as<BIT>() == ESX_FALSE;
        }
    }

    BIT SystemControlCoprocessor::areInterruptsPending() const
    {
        return mCauseRegister.get(CauseRegisterLayout::Field::IP).as<U8>() & mStatusRegister.get(StatusRegisterLayout::Field::IM).as<U8>();
    }

    BIT SystemControlCoprocessor::areInterruptsEnabled() const
    {
        return mStatusRegister.get(StatusRegisterLayout::Field::IE).as<BIT>() == ESX_TRUE &&
                mStatusRegister.get(StatusRegisterLayout::Field::EXL).as<BIT>() == ESX_FALSE && 
                mStatusRegister.get(StatusRegisterLayout::Field::ERL).as<BIT>() == ESX_FALSE;
    }

}
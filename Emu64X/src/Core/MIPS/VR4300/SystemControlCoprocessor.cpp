#include "SystemControlCoprocessor.h"

#include "VR4300.h"

namespace esx {
    SystemControlCoprocessor::SystemControlCoprocessor(VR4300* cpu)
        : Coprocessor(cpu, 0)
    {
        mRandomRegister.set<layouts::RandomRegister::Random>(31);
    }

    void SystemControlCoprocessor::clock(U64 clocks)
    {
        U32 oldCount = mCountRegister.get<layouts::CountRegister::Value>();
        U32 newCount = oldCount + 1;
        mCountRegister.set<layouts::CountRegister::Value>(newCount);
        if (newCount == mCompareRegister.get<layouts::CompareRegister::Value>()) {
            generateInterrupt(Interrupt::IP7);
        }

        U8 random = mRandomRegister.get<layouts::RandomRegister::Random>();
        U8 wired = mWiredRegister.get<layouts::WiredRegister::Wired>();
        random--;
        if (random < wired) random = wired;
        mRandomRegister.set<layouts::RandomRegister::Random>(random);

        Coprocessor::clock(clocks);
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
                break;
            }

            case 24: {
                ERET();
                break;
            }
        }
    }

    void SystemControlCoprocessor::unusable()
    {
        raiseException(ExceptionType::CoprocessorUnusable, mNumber);
    }

    void SystemControlCoprocessor::reserved()
    {
        raiseException(ExceptionType::ReservedInstruction);
    }

    void SystemControlCoprocessor::TLBR()
    {
        if (!isCoprocessorUsable(0)) {
            unusable();
            return;
        }

        U8 index = mIndexRegister.get<layouts::IndexRegister::Index>();

        TLBEntry& entry = mTLB[index];

        mPageMaskRegister.write(entry.PageMask.read());

        mEntryHiRegister.write(entry.EntryHi.read() & ~(entry.PageMask.read()));

        mEntryLo1Register.write(entry.EntryLo1.read());
        mEntryLo1Register.set<layouts::EntryLoRegister::G>(entry.EntryHi.get<layouts::EntryHiRegister::G>());

        mEntryLo0Register.write(entry.EntryLo0.read());
        mEntryLo0Register.set<layouts::EntryLoRegister::G>(entry.EntryHi.get<layouts::EntryHiRegister::G>());
    }

    void SystemControlCoprocessor::TLBWI()
    {
        if (!isCoprocessorUsable(0)) {
            unusable();
            return;
        }

        U8 index = mIndexRegister.get<layouts::IndexRegister::Index>();

        TLBEntry& entry = mTLB[index];

        entry.PageMask.write(mPageMaskRegister.read());

        entry.EntryHi.write(mEntryHiRegister.read() & ~(mPageMaskRegister.read()));
        entry.EntryHi.set<layouts::EntryHiRegister::G>(mEntryLo0Register.get<layouts::EntryLoRegister::G>() & mEntryLo1Register.get<layouts::EntryLoRegister::G>());

        entry.EntryLo1.write(mEntryLo1Register.read());
        
        entry.EntryLo0.write(mEntryLo0Register.read());
    }

    void SystemControlCoprocessor::TLBWR()
    {
        if (!isCoprocessorUsable(0)) {
            unusable();
            return;
        }

        U8 index = mRandomRegister.get<layouts::RandomRegister::Random>();

        TLBEntry& entry = mTLB[index];

        entry.PageMask.write(mPageMaskRegister.read());

        entry.EntryHi.write(mEntryHiRegister.read() & ~(mPageMaskRegister.read()));
        entry.EntryHi.set<layouts::EntryHiRegister::G>(mEntryLo0Register.get<layouts::EntryLoRegister::G>() & mEntryLo1Register.get<layouts::EntryLoRegister::G>());

        entry.EntryLo1.write(mEntryLo1Register.read());

        entry.EntryLo0.write(mEntryLo0Register.read());
    }

    void SystemControlCoprocessor::TLBP()
    {
        if (!isCoprocessorUsable(0)) {
            unusable();
            return;
        }

        U8 ASID = mEntryHiRegister.get<layouts::EntryHiRegister::ASID>();
        U8 VPN2 = mEntryHiRegister.get<layouts::EntryHiRegister::VPN2>();

        mIndexRegister.set<layouts::IndexRegister::Probe>(ESX_TRUE);
        for (U8 i = 0; i < mTLB.size(); i++) {
            TLBEntry& entry = mTLB[i];

            if (
                (entry.EntryHi.get<layouts::EntryHiRegister::VPN2>() == VPN2) &&
                (entry.EntryHi.get<layouts::EntryHiRegister::G>() == ESX_TRUE || (entry.EntryHi.get<layouts::EntryHiRegister::ASID>() == ASID))
                ) {
                mIndexRegister.set<layouts::IndexRegister::Index>(i);
                mIndexRegister.set<layouts::IndexRegister::Probe>(ESX_FALSE);
            }
        }
    }

    void SystemControlCoprocessor::ERET()
    {
        U64 PC = 0;
        if (mStatusRegister.get<layouts::StatusRegister::ERL>() == ESX_TRUE) {
            PC = mErrorEPCRegister.get<layouts::ErrorEPCRegister::Value>();
            mStatusRegister.set<layouts::StatusRegister::ERL>(ESX_FALSE);
        } else {
            PC = mEPCRegister.get<layouts::EPCRegister::Value>();
            mStatusRegister.set<layouts::StatusRegister::EXL>(ESX_FALSE);
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

    void SystemControlCoprocessor::raiseException(ExceptionType type, U32 parameter)
    {
        if (type == ExceptionType::AddressErrorLoad || type == ExceptionType::AddressErrorStore) 
            mBadVAddrRegister.set<layouts::BadVAddrRegister::Value>(parameter);
        mCauseRegister.set<layouts::CauseRegister::ExcCode>((U8)type);
        if(type == ExceptionType::CoprocessorUnusable) 
            mCauseRegister.set<layouts::CauseRegister::CE>(parameter);

        U64 EPC = 0;
        if (type == ExceptionType::Interrupt) {
            EPC = mCPU->mPC;
            mCPU->mBranchSlot = mCPU->mBranch;
            mCPU->mTookBranchSlot = mCPU->mTookBranch;
            mCPU->mNullifyBranchSlot = ESX_FALSE;
        }
        else {
            EPC = mCPU->mCurrentPC;
        }


        U32 vecOffset = 0x180;
        if (mStatusRegister.get<layouts::StatusRegister::EXL>() == ESX_FALSE) {
            if (mCPU->mBranchSlot == ESX_FALSE) {
                mCauseRegister.set<layouts::CauseRegister::BD>(ESX_FALSE);
                mEPCRegister.set<layouts::EPCRegister::Value>(EPC);
            } else {
                mCauseRegister.set<layouts::CauseRegister::BD>(ESX_TRUE);
                mEPCRegister.set<layouts::EPCRegister::Value>(EPC - 4);
            }

        }

        mStatusRegister.set<layouts::StatusRegister::EXL>(ESX_TRUE);

        if (mStatusRegister.get<layouts::StatusRegister::BEV>() == ESX_TRUE) {
            mCPU->mPC = 0xBFC00200 + vecOffset;
        } else {
            mCPU->mPC = 0x80000000 + vecOffset;
        }

        mCPU->mPC = static_cast<I32>(mCPU->mPC);
        mCPU->mNextPC = mCPU->mPC + 4;
    }

    void SystemControlCoprocessor::raiseTLBException(TLBExceptionType type, U32 virtualAddress)
    {
        if (type == TLBExceptionType::TLBMissLoadFetch || type == TLBExceptionType::TLBMissStore) {
            mContextRegister.set<layouts::ContextRegister::BadVPN2>(virtualAddress >> 13);
            mBadVAddrRegister.set<layouts::BadVAddrRegister::Value>(virtualAddress);
        }

        if (type == TLBExceptionType::TLBMissLoadFetch || type == TLBExceptionType::TLBMissStore) {
            mXContextRegister.set<layouts::XContextRegister::BadVPN2>(virtualAddress >> 13);
            mXContextRegister.set<layouts::XContextRegister::R>((virtualAddress >> 62) & 0x3);
        }

        mCauseRegister.set<layouts::CauseRegister::ExcCode>((U8)type);

        U32 vecOffset = 0x000;
        if (mStatusRegister.get<layouts::StatusRegister::EXL>() == ESX_FALSE) {
            if (mCPU->mBranchSlot == ESX_FALSE) {
                mCauseRegister.set<layouts::CauseRegister::BD>(ESX_FALSE);
                mEPCRegister.set<layouts::EPCRegister::Value>(mCPU->mCurrentPC);
            }
            else {
                mCauseRegister.set<layouts::CauseRegister::BD>(ESX_TRUE);
                mEPCRegister.set<layouts::EPCRegister::Value>(mCPU->mCurrentPC - 4);
            }

            vecOffset = is64BitMode() ? 0x080 : 0x000;
        } else {
            vecOffset = 0x180;
        }

        mStatusRegister.set<layouts::StatusRegister::EXL>(ESX_TRUE);

        if (mStatusRegister.get<layouts::StatusRegister::BEV>() == ESX_TRUE) {
            mCPU->mPC = 0xBFC00200 + vecOffset;
        }
        else {
            mCPU->mPC = 0x80000000 + vecOffset;
        }

        mCPU->mPC = static_cast<I32>(mCPU->mPC);
        mCPU->mNextPC = mCPU->mPC + 4;
    }

    void SystemControlCoprocessor::watchAddress(U32 physicalAddress, BIT store)
    {
        if (store == ESX_TRUE && mWatchLoRegister.get<layouts::WatchLoRegister::W>() == ESX_TRUE && mWatchLoRegister.get<layouts::WatchLoRegister::PAddr0>() == physicalAddress >> 3)
            raiseException(ExceptionType::Watch);

        if (store == ESX_FALSE && mWatchLoRegister.get<layouts::WatchLoRegister::R>() == ESX_TRUE && mWatchLoRegister.get<layouts::WatchLoRegister::PAddr0>() == physicalAddress >> 3)
            raiseException(ExceptionType::Watch);
    }

    U32 SystemControlCoprocessor::AddressTranslation(U32 virtualAddress, BIT store, BIT& cached)
    {
        U32 physicalAddress = 0;

        if (isAddressLegal(virtualAddress) == ESX_FALSE) {
            raiseException(store ? ExceptionType::AddressErrorStore :ExceptionType::AddressErrorLoad, virtualAddress);
        }

        if (isAdressMapped(virtualAddress) == ESX_TRUE) {
            U8 pageSelect = (virtualAddress >> 12) & 0x1;
            U16 pageOffset = virtualAddress & 0xFFF;

            U8 ASID = mEntryHiRegister.get<layouts::EntryHiRegister::ASID>();

            BIT matchFound = ESX_FALSE;
            for (U8 i = 0; i < mTLB.size(); i++) {
                TLBEntry& entry = mTLB[i];

                U32 VPN2 = (virtualAddress & ~entry.PageMask.read()) >> 13;

                if (
                    (entry.EntryHi.get<layouts::EntryHiRegister::VPN2>() == VPN2) &&
                    (entry.EntryHi.get<layouts::EntryHiRegister::G>() == ESX_TRUE || (entry.EntryHi.get<layouts::EntryHiRegister::ASID>() == ASID))
                    ) {

                    EntryLoRegister& lo = pageSelect == 0 ? entry.EntryLo0 : entry.EntryLo1;

                    if (lo.get<layouts::EntryLoRegister::V>() == ESX_FALSE) {
                        raiseTLBException(store ? TLBExceptionType::TLBInvalidStore : TLBExceptionType::TLBInvalidLoadFetch, virtualAddress);
                        break;
                    }

                    if (store == ESX_TRUE && lo.get<layouts::EntryLoRegister::D>() == ESX_FALSE) {
                        raiseTLBException(TLBExceptionType::TLBMod, virtualAddress);
                        break;
                    }

                    physicalAddress = (lo.get<layouts::EntryLoRegister::PFN>() << 12) | pageOffset;
                    cached = lo.get<layouts::EntryLoRegister::C>();

                    matchFound = ESX_TRUE;

                    break;
                }
            }

            if (matchFound == ESX_FALSE) {
                raiseTLBException(store ? TLBExceptionType::TLBMissStore : TLBExceptionType::TLBMissLoadFetch, virtualAddress);
            }
        } else {
            physicalAddress = virtualAddress & 0x1FFFFFFF;
            cached = ESX_TRUE;
        }

        mLastPhysicalAddress = physicalAddress;

        return physicalAddress;
    }

    U64 SystemControlCoprocessor::getRegister(RegisterIndex reg)
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
                mRandomRegister.set<layouts::RandomRegister::Random>(31);
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

            case SystemControlRegisterType::Cause: {
                CauseRegister temp;
                temp.write(value);

                mCauseRegister.set<layouts::CauseRegister::IP>((mCauseRegister.get<layouts::CauseRegister::IP>() & ~0x83) | (temp.get<layouts::CauseRegister::IP>() & 0x3));
                break;
            }

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
            default: break;
        }
    }

    void SystemControlCoprocessor::clearInterrupt(Interrupt interrupt)
    {
        mCauseRegister.set<layouts::CauseRegister::IP>(mCauseRegister.get<layouts::CauseRegister::IP>() & (~static_cast<U8>(interrupt)));
    }

    void SystemControlCoprocessor::generateInterrupt(Interrupt interrupt)
    {
        mCauseRegister.set<layouts::CauseRegister::IP>(mCauseRegister.get<layouts::CauseRegister::IP>() | static_cast<U8>(interrupt));
    }

    OperatingMode SystemControlCoprocessor::getCurrentOperatingMode() const
    {
        if (mStatusRegister.get<layouts::StatusRegister::EXL>() == ESX_TRUE || mStatusRegister.get<layouts::StatusRegister::ERL>() == ESX_TRUE) {
            return OperatingMode::Kernel;
        }

        return static_cast<OperatingMode>(mStatusRegister.get<layouts::StatusRegister::KSU>());
    }

    BIT SystemControlCoprocessor::isAddressLegal(U32 virtualAddress) const
    {
        U8 segment = virtualAddress >> 29;

        switch (getCurrentOperatingMode()) {
            case OperatingMode::Kernel: return ESX_TRUE;
            case OperatingMode::Supervisor: return (segment >= 0 && segment <= 3) || segment == 6;
            case OperatingMode::User: return segment >= 0 && segment <= 3;
        }
        return ESX_FALSE;
    }

    BIT SystemControlCoprocessor::isAdressMapped(U32 virtualAddress) const
    {
        U8 segment = virtualAddress >> 29;
        return (segment >= 0 && segment <= 3) || segment >= 6;
    }

    BIT SystemControlCoprocessor::is64BitMode() const
    {
        switch (getCurrentOperatingMode()) {
            case OperatingMode::Kernel: return mStatusRegister.get<layouts::StatusRegister::KX>();
            case OperatingMode::Supervisor: return mStatusRegister.get<layouts::StatusRegister::SX>();
            case OperatingMode::User: return mStatusRegister.get<layouts::StatusRegister::UX>();
        }
        return ESX_FALSE;
    }

    BIT SystemControlCoprocessor::isCoprocessorUsable(U8 copNumber) const
    {
        if (copNumber == 0 && getCurrentOperatingMode() == OperatingMode::Kernel)
            return ESX_TRUE;

        return mStatusRegister.get<layouts::StatusRegister::CU>() & (1 << copNumber);
    }

    BIT SystemControlCoprocessor::isReserved64BitInstruction() const
    {
        switch (getCurrentOperatingMode()) {
            case OperatingMode::Kernel: return ESX_FALSE;
            case OperatingMode::Supervisor: return mStatusRegister.get<layouts::StatusRegister::SX>() == ESX_FALSE;
            case OperatingMode::User: return mStatusRegister.get<layouts::StatusRegister::UX>() == ESX_FALSE;
        }
        return ESX_FALSE;
    }

    BIT SystemControlCoprocessor::areInterruptsPending() const
    {
        return mCauseRegister.get<layouts::CauseRegister::IP>() & mStatusRegister.get<layouts::StatusRegister::IM>();
    }

    BIT SystemControlCoprocessor::areInterruptsEnabled() const
    {
        return mStatusRegister.get<layouts::StatusRegister::IE>() == ESX_TRUE &&
                mStatusRegister.get<layouts::StatusRegister::EXL>() == ESX_FALSE &&
                mStatusRegister.get<layouts::StatusRegister::ERL>() == ESX_FALSE;
    }

    void SystemControlCoprocessor::setLLAddrToLastTranslation()
    {
        mLLAddrRegister.set<layouts::LLAddrRegister::Value>(mLastPhysicalAddress);
    }

    BIT SystemControlCoprocessor::useAdditionalFPR()
    {
        return mStatusRegister.get<layouts::StatusRegister::FR>();
    }

}
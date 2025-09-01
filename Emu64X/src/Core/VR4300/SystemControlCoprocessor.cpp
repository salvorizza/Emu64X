#include "SystemControlCoprocessor.h"

#include "VR4300.h"

namespace esx {
	
    void SystemControlCoprocessor::CO(VR4300* cpu)
    {
        switch (cpu->mCurrentInstruction.CoprocessorFunction()) {
            case 1: {
                TLBR(cpu);
                break;
            }

            case 2: {
                TLBWI(cpu);
                break;
            }

            case 6: {
                TLBWR(cpu);
                break;
            }

            case 8: {
                TLBP(cpu);
                break;
            }

            case 16: {
                cpu->raiseException(ExceptionType::ReservedInstruction);
                break;
            }

            case 24: {
                ERET(cpu);
                break;
            }
        }
    }

    void SystemControlCoprocessor::ERET(VR4300* cpu)
    {
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
        case SystemControlRegisterType::Wired:    mWiredRegister.write(value); break;
        case SystemControlRegisterType::Count:    mCountRegister.write(value); break;
        case SystemControlRegisterType::EntryHi:  mEntryHiRegister.write(value); break;
        case SystemControlRegisterType::Compare:  mCompareRegister.write(value); break;
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

    BIT SystemControlCoprocessor::is64BitMode() const
    {
        switch (mStatusRegister.get(StatusRegisterLayout::Field::KSU).as<U8>()) {
        case 0: return mStatusRegister.get(StatusRegisterLayout::Field::KX).as<BIT>();
        case 1: return mStatusRegister.get(StatusRegisterLayout::Field::SX).as<BIT>();
        case 2: return mStatusRegister.get(StatusRegisterLayout::Field::UX).as<BIT>();
        }
    }

    BIT SystemControlCoprocessor::isCoprocessorUsable(U8 copNumber) const
    {
        if (mStatusRegister.get(StatusRegisterLayout::Field::KSU).as<U8>() == 0)
            return ESX_TRUE;

        return mStatusRegister.get(StatusRegisterLayout::Field::CU).as<U8>() & (1 << copNumber);
    }

    BIT SystemControlCoprocessor::isReserved64BitInstruction() const
    {
        switch (mStatusRegister.get(StatusRegisterLayout::Field::KSU).as<U8>()) {
        case 0: return ESX_FALSE;
        case 1: return mStatusRegister.get(StatusRegisterLayout::Field::SX).as<BIT>() == ESX_FALSE;
        case 2: return mStatusRegister.get(StatusRegisterLayout::Field::UX).as<BIT>() == ESX_FALSE;
        }
    }

}
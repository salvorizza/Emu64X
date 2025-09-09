#include "RCP.h"

//	addRange(ESX_TEXT("Root"), 0x04300000, 0x000FFFFF, 0xFFFFFFFF); MI
//	addRange(ESX_TEXT("Root"), 0x04400000, 0x000FFFFF, 0xFFFFFFFF); VI
//	addRange(ESX_TEXT("Root"), 0x04500000, 0x000FFFFF, 0xFFFFFFFF); AI
//	addRange(ESX_TEXT("Root"), 0x04600000, 0x000FFFFF, 0xFFFFFFFF); PI
//	addRange(ESX_TEXT("Root"), 0x04700000, 0x000FFFFF, 0xFFFFFFFF); RI
//	addRange(ESX_TEXT("Root"), 0x04800000, 0x000FFFFF, 0xFFFFFFFF); SI

namespace esx {
	RCP::RCP()
		: BusDevice("RCP")
	{
		addRange(ESX_TEXT("Root"), 0x04000000, 0x00FFFFFF, 0xFFFFFFFF);
	}

	RCP::~RCP()
	{
	}

	void RCP::clock(U64 clocks)
	{
	}

	void RCP::init()
	{
		mIMEM.resize(KIBI(4));
		mDMEM.resize(KIBI(4));

		mRSP = MakeShared<RSP>(this);

		mAudioInterface = MakeShared<AudioInterface>(this);
		mMIPSInterface = MakeShared<MIPSInterface>(this);
		mPeripheralInterface = MakeShared<PeripheralInterface>(this);
		mRDRAMInterface = MakeShared<RDRAMInterface>(this);
		mSerialInterface = MakeShared<SerialInterface>(this);
		mVideoInterface = MakeShared<VideoInterface>(this);

		mRSP->init();
		mAudioInterface->init();
		mMIPSInterface->init();
		mPeripheralInterface->init();
		mRDRAMInterface->init();
		mSerialInterface->init();
		mVideoInterface->init();

		mRoot = getBus("Root");
	}

	U32 RCP::SysADLoad(U32 address, U8 accessSize)
	{
		return mRoot->load(address & 0xFFFFFFFC);
	}

	void RCP::SysADStore(U32 address, U8 accessSize, U32 value)
	{
		return mRoot->store(address & 0xFFFFFFFC, value);
	}

	void RCP::store(const StringView& busName, U32 address, U32 value)
	{
		if (address >= 0x04000000 && address <= 0x04000FFF) {
			*reinterpret_cast<U32*>(&mDMEM[address - 0x04000000]) = _byteswap_ulong(value);
		} 
		else if (address >= 0x04001000 && address <= 0x04001FFF) {
			*reinterpret_cast<U32*>(&mIMEM[address - 0x04001000]) = _byteswap_ulong(value);
		}
		else if (address >= 0x04040000 && address <= 0x040BFFFF) {
			mRSP->store(address, value);
		}
		else if (address >= 0x040C0000 && address <= 0x040FFFFF) {
			//TODO: Freeze
		}
		else if (address >= 0x04100000 && address <= 0x041FFFFF) {
			//TODO: RDP Command
		}
		else if (address >= 0x04200000 && address <= 0x042FFFFF) {
			//TODO: RDP Span
		}
		else if (address >= 0x04300000 && address <= 0x043FFFFF) {
			mMIPSInterface->store(address, value);
		}
		else if (address >= 0x04400000 && address <= 0x044FFFFF) {
			mVideoInterface->store(address, value);
		}
		else if (address >= 0x04500000 && address <= 0x045FFFFF) {
			mAudioInterface->store(address, value);
		}
		else if (address >= 0x04600000 && address <= 0x046FFFFF) {
			mPeripheralInterface->store(address, value);
		}
		else if (address >= 0x04700000 && address <= 0x047FFFFF) {
			mRDRAMInterface->store(address, value);
		}
		else if (address >= 0x04800000 && address <= 0x048FFFFF) {
			mSerialInterface->store(address, value);
		}
		else if (address >= 0x04900000 && address <= 0x04FFFFFF) {
			//TODO: Freeze
		}
	}

	void RCP::load(const StringView& busName, U32 address, U32& output)
	{
		output = 0;

		if (address >= 0x04000000 && address <= 0x04000FFF) {
			output = _byteswap_ulong(*reinterpret_cast<U32*>(&mDMEM[address - 0x04000000]));
		}
		else if (address >= 0x04001000 && address <= 0x04001FFF) {
			output = _byteswap_ulong(*reinterpret_cast<U32*>(&mIMEM[address - 0x04001000]));
		}
		else if (address >= 0x04040000 && address <= 0x040BFFFF) {
			output = mRSP->load(address);
		}
		else if (address >= 0x040C0000 && address <= 0x040FFFFF) {
			//TODO: Freeze
		}
		else if (address >= 0x04100000 && address <= 0x041FFFFF) {
			//TODO: RDP Command
		}
		else if (address >= 0x04200000 && address <= 0x042FFFFF) {
			//TODO: RDP Span
		}
		else if (address >= 0x04300000 && address <= 0x043FFFFF) {
			output = mMIPSInterface->load(address);
		}
		else if (address >= 0x04400000 && address <= 0x044FFFFF) {
			output = mVideoInterface->load(address);
		}
		else if (address >= 0x04500000 && address <= 0x045FFFFF) {
			output = mAudioInterface->load(address);
		}
		else if (address >= 0x04600000 && address <= 0x046FFFFF) {
			output = mPeripheralInterface->load(address);
		}
		else if (address >= 0x04700000 && address <= 0x047FFFFF) {
			output = mRDRAMInterface->load(address);
		}
		else if (address >= 0x04800000 && address <= 0x048FFFFF) {
			output = mSerialInterface->load(address);
		}
		else if (address >= 0x04900000 && address <= 0x04FFFFFF) {
			//TODO: Freeze
		}
	}

	void RCP::reset()
	{
		std::fill(mIMEM.begin(), mIMEM.end(), 0);
		std::fill(mDMEM.begin(), mDMEM.end(), 0);

		mRSP->reset();
		mAudioInterface->reset();
		mMIPSInterface->reset();
		mPeripheralInterface->reset();
		mRDRAMInterface->reset();
		mSerialInterface->reset();
		mVideoInterface->reset();
	}

	void RCP::setInterrupt(InterruptType type, BIT prevValue, BIT newValue, U64 delay) {
		mMIPSInterface->setInterrupt(type, prevValue, newValue, delay);
	}

	void RCP::clearInterrupt(InterruptType type) {
		mMIPSInterface->clearInterrupt(type);
	}
}
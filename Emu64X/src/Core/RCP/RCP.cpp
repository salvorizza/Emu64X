#include "RCP.h"

#include "Core/MIPS/VR4300/VR4300.h"

namespace esx {
	RCP::RCP()
		:	BusDevice("RCP")
	{
		addRange(ESX_TEXT("Root"), 0x04000000, 0x00FFFFFF, 0xFFFFFFFF);
	}

	RCP::~RCP()
	{
	}

	void RCP::clock(U64 clocks)
	{
		mRSP->clock(clocks);
		mMIPSInterface->clock(clocks);
	}

	void RCP::init()
	{
		mIMEM.resize(KIBI(4));
		mDMEM.resize(KIBI(4));

		mRSP = MakeShared<RSP>(this);
		mRDP = MakeShared<RDP>(this);

		mAudioInterface = MakeShared<AudioInterface>(this);
		mMIPSInterface = MakeShared<MIPSInterface>(this);
		mPeripheralInterface = MakeShared<PeripheralInterface>(this);
		mRDRAMInterface = MakeShared<RDRAMInterface>(this);
		mSerialInterface = MakeShared<SerialInterface>(this);
		mVideoInterface = MakeShared<VideoInterface>(this);

		mRSP->init();
		mRDP->init();
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
		return mRoot->load(address & 0xFFFFFFFC, address & 0x3, accessSize);
	}

	void RCP::SysADStore(U32 address, U8 accessSize, U32 value)
	{
		return mRoot->store(address & 0xFFFFFFFC, value, address & 0x3, accessSize);
	}

	void RCP::store(const StringView& busName, U32 address, U32 value, U8 lowerBits, U8 accessSize)
	{
		if (address >= 0x04000000 && address <= 0x04000FFF) {
			switch (accessSize) {
				case sizeof(U8) * 8: *reinterpret_cast<U8*>(&mDMEM[address - 0x04000000]) = value; break;
				case sizeof(U16) * 8: *reinterpret_cast<U16*>(&mDMEM[address - 0x04000000]) = _byteswap_ushort(value); break;
				case sizeof(U32) * 8: *reinterpret_cast<U32*>(&mDMEM[address - 0x04000000]) = _byteswap_ulong(value); break;
			}
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
			U32 reg = (address - 0x04100000) >> 2;
			mRSP->storeDPCRegister(reg, value);
		}
		else if (address >= 0x04200000 && address <= 0x042FFFFF) {
			//TODO: RDP Span
			ESX_CORE_LOG_WARNING("RDP span not implemented yet");
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

	void RCP::load(const StringView& busName, U32 address, U32& output, U8 lowerBits, U8 accessSize)
	{
		output = 0;

		if (address >= 0x04000000 && address <= 0x04000FFF) {
			switch (accessSize) {
				case sizeof(U8) * 8 : output = *reinterpret_cast<U8*>(&mDMEM[address - 0x04000000]); break;
				case sizeof(U16) * 8: output = _byteswap_ushort(*reinterpret_cast<U16*>(&mDMEM[address - 0x04000000])); break;
				case sizeof(U32) * 8: output = _byteswap_ulong(*reinterpret_cast<U32*>(&mDMEM[address - 0x04000000])); break;
			}
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
			U32 reg = (address - 0x04100000) >> 2;
			output = static_cast<U32>(mRSP->loadDPCRegister(reg));
		}
		else if (address >= 0x04200000 && address <= 0x042FFFFF) {
			//TODO: RDP Span
			ESX_CORE_LOG_WARNING("RDP span not implemented yet");
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
		mRDP->reset();
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

	BIT RCP::interruptPending()
	{
		return mMIPSInterface->interruptPending();
	}

	U64 RCP::RCPClocksToCPUClocks(U64 RCPClocks)
	{
		return (RCPClocks * 93750000llu) / 62500000llu;
	}

	U64 RCP::CPUClocksToRCPClocks(U64 CPUClocks)
	{
		return (CPUClocks * 62500000llu) / 93750000llu;
	}
}
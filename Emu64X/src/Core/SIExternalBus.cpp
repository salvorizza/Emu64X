#include "SIExternalBus.h"

#include <intrin.h>

namespace esx {

	struct CICData {
		CIC Chip;
		U8 IPL2Seed;
		U64 IPL2Checksum;
		U8 IPL3Seed;
		U32 IPL3Magic;
		U32 IPL3InitialChecksum;
	};

	static UnorderedMap<CIC, CICData> sCICData = {
		{CIC::Chip6101, {.Chip = CIC::Chip6101, .IPL2Seed = 0x3F, .IPL2Checksum = 0x45CC73EE317A, .IPL3Seed = 0x3F, .IPL3Magic = 0x5D588B65, .IPL3InitialChecksum = 0xF8CA4DDC}},
		{CIC::Chip6102, {.Chip = CIC::Chip6102, .IPL2Seed = 0x3F, .IPL2Checksum = 0xA536C0F1D859, .IPL3Seed = 0x3F, .IPL3Magic = 0x5D588B65, .IPL3InitialChecksum = 0xF8CA4DDC}},
		{CIC::Chip7101, {.Chip = CIC::Chip7101, .IPL2Seed = 0x3F, .IPL2Checksum = 0xA536C0F1D859, .IPL3Seed = 0x3F, .IPL3Magic = 0x5D588B65, .IPL3InitialChecksum = 0xF8CA4DDC}},
		{CIC::Chip7102, {.Chip = CIC::Chip7102, .IPL2Seed = 0x3F, .IPL2Checksum = 0x44160EC5D9AF, .IPL3Seed = 0x3F, .IPL3Magic = 0x5D588B65, .IPL3InitialChecksum = 0xF8CA4DDC}},
		{CIC::Chip6103, {.Chip = CIC::Chip6103, .IPL2Seed = 0x78, .IPL2Checksum = 0x586FD4709867, .IPL3Seed = 0x78, .IPL3Magic = 0x6C078965, .IPL3InitialChecksum = 0xA3886759}},
		{CIC::Chip7103, {.Chip = CIC::Chip7103, .IPL2Seed = 0x78, .IPL2Checksum = 0x586FD4709867, .IPL3Seed = 0x78, .IPL3Magic = 0x6C078965, .IPL3InitialChecksum = 0xA3886759}},
		{CIC::Chip6105, {.Chip = CIC::Chip6105, .IPL2Seed = 0x91, .IPL2Checksum = 0x8618A45BC2D3, .IPL3Seed = 0x91, .IPL3Magic = 0x5D588B65, .IPL3InitialChecksum = 0xDF26F436}},
		{CIC::Chip7105, {.Chip = CIC::Chip7105, .IPL2Seed = 0x91, .IPL2Checksum = 0x8618A45BC2D3, .IPL3Seed = 0x91, .IPL3Magic = 0x5D588B65, .IPL3InitialChecksum = 0xDF26F436}},
		{CIC::Chip6106, {.Chip = CIC::Chip6106, .IPL2Seed = 0x85, .IPL2Checksum = 0x2BBAD4E6EB74, .IPL3Seed = 0x85, .IPL3Magic = 0x6C078965, .IPL3InitialChecksum = 0x1FEA617A}},
		{CIC::Chip7106, {.Chip = CIC::Chip7106, .IPL2Seed = 0x85, .IPL2Checksum = 0x2BBAD4E6EB74, .IPL3Seed = 0x85, .IPL3Magic = 0x6C078965, .IPL3InitialChecksum = 0x1FEA617A}}
	};

	SIExternalBus::SIExternalBus(StringView path)
		:	BusDevice(ESX_TEXT("SIExternalBus")),
			mPIF_Path(path),
			mCIC(CIC::Chip6102)
	{
		addRange(ESX_TEXT("Root"), 0x1FC00000, 0xFFFFF, 0xFFFFFFFF);
	}

	SIExternalBus::~SIExternalBus()
	{
	}

	void SIExternalBus::init()
	{
		std::ifstream input(mPIF_Path.data(), std::ios::binary);
		mPIF_ROM.insert(mPIF_ROM.begin(), std::istreambuf_iterator<char>(input), {});
		mPIF_ROM.resize(KIBI(2));
		input.close();

		mPIF_RAM.resize(0x40);

		mJoybusAddr.resize(5);
		mJoybusStatus.resize(5);
	}

	void SIExternalBus::load(const StringView& busName, U32 address, U32& output, U8 lowerBits, U8 accessSize)
	{
		if (address >= 0x1FC00000 && address <= 0x1FC007BF) {
			output = mLockPIF_ROM == ESX_TRUE ? 0x00000000 : _byteswap_ulong(*reinterpret_cast<U32*>(&mPIF_ROM[address - 0x1FC00000]));
		}
		else if (address >= 0x1FC007C0 && address <= 0x1FC007FF) {
			output = _byteswap_ulong(*reinterpret_cast<U32*>(&mPIF_RAM[address - 0x1FC007C0]));
		}
		else if (address >= 0x1FC00800 && address <= 0x1FCFFFFF) {
			ESX_CORE_LOG_ERROR("Load {} - Reserved address 0x{:08x}", mName, address);
		}
	}

	void SIExternalBus::store(const StringView& busName, U32 address, U32 value, U8 lowerBits, U8 accessSize)
	{
		if (address >= 0x1FC00000 && address <= 0x1FC007BF) {
			ESX_CORE_LOG_ERROR("Store {} - Read only address 0x{:08x}", mName, address);
		}
		else if (address >= 0x1FC007C0 && address <= 0x1FC007FF) {
			*reinterpret_cast<U32*>(&mPIF_RAM[address - 0x1FC007C0]) = _byteswap_ulong(value);

			U8 commandByte = mPIF_RAM[0x3F];

			auto manageCommand = [&](PIF_CommandBits command, auto callback) {
				if ((commandByte & static_cast<U8>(command)) != 0) {
					callback();
					commandByte &= ~(static_cast<U8>(command));
				}
			};

			if (commandByte != 0) {
				commandByte &= ~(static_cast<U8>(PIF_CommandBits::Complete));

				manageCommand(PIF_CommandBits::TerminateBootProcess,
					[&]() {
						ESX_CORE_LOG_INFO("PIF TerminateBootProcess received");
					}
				);

				manageCommand(PIF_CommandBits::ROMLockout,
					[&]() {
						mLockPIF_ROM = ESX_TRUE; 
					}
				);

				manageCommand(PIF_CommandBits::AcquireChecksum,
					[&]() { 
						mAcquiredChecksum = _byteswap_uint64(*reinterpret_cast<U64*>(&mPIF_RAM[0x30]));  
						*reinterpret_cast<U64*>(&mPIF_RAM[0x30]) = 0;

						commandByte |= static_cast<U8>(PIF_CommandBits::Complete);
					}
				);

				manageCommand(PIF_CommandBits::RunChecksum,
					[&]() {
						if (mAcquiredChecksum != sCICData.at(mCIC).IPL2Checksum) {
							ESX_CORE_LOG_ERROR("PIF different checksum Acquired => {:016x}h, IPL2Checksum => {:016x}h halt not implemented yet.", mAcquiredChecksum, sCICData.at(mCIC).IPL2Checksum);
						}
					}
				);

				manageCommand(PIF_CommandBits::ConfigureJoybusFrame,
					[&]() {
						ParseJoybusFrame();
					}
				);

				manageCommand(PIF_CommandBits::Challenge,
					[&]() {
						ESX_CORE_LOG_WARNING("PIF Challenge not implemented yet");
					}
				);


				mPIF_RAM[0x3F] = commandByte;
			}
		}
		else if (address >= 0x1FC00800 && address <= 0x1FCFFFFF) {
			ESX_CORE_LOG_ERROR("Store {} - Reserved address 0x{:08x}", mName, address);
		}
	}


	void SIExternalBus::reset()
	{
		std::fill(mPIF_RAM.begin(), mPIF_RAM.end(), 0);

		const CICData& cicData = sCICData.at(mCIC);
		*reinterpret_cast<U32*>(&mPIF_RAM[0x1FC007E4 - 0x1FC007C0]) = _byteswap_ulong(static_cast<U32>(cicData.IPL2Seed) << 8 | cicData.IPL3Seed);

		mAcquiredChecksum = 0;
		mLockPIF_ROM = ESX_FALSE;
	}

	void SIExternalBus::setCIC(CIC cic)
	{
		mCIC = cic;
	}

	void SIExternalBus::ParseJoybusFrame()
	{
		std::fill(mJoybusStatus.begin(), mJoybusStatus.end(), (U8)JoybusStatus::Skip);

		U8 nibble = 0;
		U8 nChannels = 0;

		do {
			U8 tx = mPIF_RAM[nibble / 2];

			if (tx == 0xFE) {
				break;
			}

			if (tx == 0xFD) {
				mJoybusStatus[nChannels] |= ((U8)JoybusStatus::Reset);
			}

			if (tx == 0xFF || tx == 0xFD || tx == 0x00) {
				nibble += 2;
				if (!nibble) {
					break;
				}

				if (tx != 0xFF)
					++nChannels;
			}
			else {
				mJoybusAddr[nChannels] = nibble / 2;
				mJoybusStatus[nChannels] &= ~((U8)JoybusStatus::Skip);
				++nChannels;

				BIT returnValue = ESX_FALSE;
				U8 address = nibble;

				U8 send = mPIF_RAM[address / 2] & 0x3F;
				address += 2;
				if (!address) {
					returnValue = ESX_TRUE;
				}

				if (returnValue == ESX_FALSE) {
					U8 recv = mPIF_RAM[address / 2] & 0x3F;
					U8 count = send;
					count += recv;
					count += count;

					U16 next = address + count + 2;
					if (next >= (mPIF_RAM.size() * 2)) {
						returnValue = ESX_TRUE;
					}

					if (returnValue == ESX_FALSE) {
						nibble = next;
					}
				}

				if (returnValue == ESX_TRUE) {
					nChannels--;
					mJoybusStatus[nChannels] &= ~((U8)JoybusStatus::Reset);
					mJoybusStatus[nChannels] |= ((U8)JoybusStatus::Skip);
					break;
				}
			}
		} while (nChannels != 5);
	}

	void SIExternalBus::ExecuteJoybusFrame()
	{
		for (I32 nChannel = 0; nChannel < 5; nChannel++) {
			if ((mJoybusStatus[nChannel] & static_cast<U8>(JoybusStatus::Reset)) != 0) {
				ESX_CORE_LOG_WARNING("JoybusFrame Reset not implemented yet");
			}

			if ((mJoybusStatus[nChannel] & static_cast<U8>(JoybusStatus::Skip)) != 0) {
				continue;
			}

			U8 addr = mJoybusAddr[nChannel];

			U8 tx = mPIF_RAM[addr++];
			U8 rx = mPIF_RAM[addr++];
			if (tx > 0) {
				U8 command = mPIF_RAM[addr++];
				Span<U8> parameters;
				if (tx > 1) {
					parameters = Span<U8>(mPIF_RAM.begin() + addr, mPIF_RAM.begin() + addr + (tx - 1));
					addr += (tx - 1);
				}

				switch (command) {
					case 0xFF:
					case 0x00: {
						for (I32 i = 0; i < rx; i++) {
							mPIF_RAM[addr++] = 0x00;
						}
						break;
					}

					default: {
						ESX_CORE_LOG_WARNING("JoybusFrame tx => {:02x}h not implemented yet", tx);
						break;
					}
				}
			}

		}
	}

}
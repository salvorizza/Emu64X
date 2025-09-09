#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

#include "RSP/RSP.h"
#include "Interfaces/AudioInterface.h"
#include "Interfaces/MIPSInterface.h"
#include "Interfaces/PeripheralInterface.h"
#include "Interfaces/RDRAMInterface.h"
#include "Interfaces/SerialInterface.h"
#include "Interfaces/VideoInterface.h"

namespace esx {

	class RCP : public BusDevice {
	public:
		RCP();
		~RCP();

		void clock(U64 clocks) override;

		void init() override;

		U32 SysADLoad(U32 address, U8 accessSize);
		void SysADStore(U32 address, U8 accessSize, U32 value);

		void store(const StringView& busName, U32 address, U32 value) override;
		void load(const StringView& busName, U32 address, U32& output) override;

		void reset() override;

		void setInterrupt(InterruptType type, BIT prevValue, BIT newValue, U64 delay);
		void clearInterrupt(InterruptType type);
	private:
		SharedPtr<Bus> mRoot;

		Vector<U8> mIMEM;
		Vector<U8> mDMEM;
		SharedPtr<RSP> mRSP;
		SharedPtr<AudioInterface> mAudioInterface;
		SharedPtr<MIPSInterface> mMIPSInterface;
		SharedPtr<PeripheralInterface> mPeripheralInterface;
		SharedPtr<RDRAMInterface> mRDRAMInterface;
		SharedPtr<SerialInterface> mSerialInterface;
		SharedPtr<VideoInterface> mVideoInterface;

	};

}

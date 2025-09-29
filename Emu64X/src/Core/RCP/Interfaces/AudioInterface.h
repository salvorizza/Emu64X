#pragma once

#define AI_DRAM_ADDR_FIELDS(M) M(DRAM_ADDR, 3, 23)
#define AI_LENGTH_FIELDS(M)    M(LENGTH, 3, 17)
#define AI_CONTROL_FIELDS(M)   M(DMA_ENABLE, 0, 0)
#define AI_STATUS_FIELDS(M)    M(FULL_COPY, 0, 0)  M(COUNT, 1, 14)  M(BC, 16, 16)  M(WC, 19, 19)  M(ENABLED, 25, 25)  M(BUSY, 30, 30)  M(FULL,31,31)
#define AI_DACRATE_FIELDS(M)   M(DACRATE, 0, 13)
#define AI_BITRATE_FIELDS(M)   M(BITRATE, 0, 3)

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	class RCP;

	DEFINE_REGISTER_LAYOUT(AI_DRAM_ADDR_Register, U32, AI_DRAM_ADDR_FIELDS)
	DEFINE_REGISTER_LAYOUT(AI_LENGTH_Register, U32, AI_LENGTH_FIELDS)
	DEFINE_REGISTER_LAYOUT(AI_CONTROL_Register, U32, AI_CONTROL_FIELDS)
	DEFINE_REGISTER_LAYOUT(AI_STATUS_Register, U32, AI_STATUS_FIELDS)
	DEFINE_REGISTER_LAYOUT(AI_DACRATE_Register, U32, AI_DACRATE_FIELDS)
	DEFINE_REGISTER_LAYOUT(AI_BITRATE_Register, U32, AI_BITRATE_FIELDS)

	class AudioInterface {
	public:
		AudioInterface(RCP* rcp);
		~AudioInterface();

		void init();
		void clock(U64 clocks);

		void store(U32 address, U32 value);
		U32 load(U32 address);

		void reset();
	private:
		StringView mName = "AudioInterface";

		RCP* mRCP;

		AI_DRAM_ADDR_Register AI_DRAM_ADDR;
		AI_LENGTH_Register AI_LENGTH;
		AI_CONTROL_Register AI_CONTROL;
		AI_STATUS_Register AI_STATUS;
		AI_DACRATE_Register AI_DACRATE;
		AI_BITRATE_Register AI_BITRATE;
	};

}
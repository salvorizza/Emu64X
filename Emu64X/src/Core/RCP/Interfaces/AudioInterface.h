#pragma once

#define AI_DRAM_ADDR_FIELDS(M) M(DRAM_ADDR, 3, 23)
#define AI_LENGTH_FIELDS(M)    M(LENGTH, 3, 17)

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	class RCP;

	DEFINE_REGISTER_LAYOUT(AI_DRAM_ADDR_Register, U32, AI_DRAM_ADDR_FIELDS)
	DEFINE_REGISTER_LAYOUT(AI_LENGTH_Register, U32, AI_LENGTH_FIELDS)

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
	};

}
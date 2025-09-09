#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	class RCP;

	struct AI_DRAM_ADDR_RegisterLayout {
		enum class Field { DRAM_ADDR };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
				case Field::DRAM_ADDR: return { 3, 23 };
			}
		}
	};
	using AI_DRAM_ADDR_Register = Register<AI_DRAM_ADDR_RegisterLayout, U32>;

	struct AI_LENGTH_RegisterLayout {
		enum class Field { LENGTH };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::LENGTH: return { 3, 17 };
			}
		}
	};
	using AI_LENGTH_Register = Register<AI_LENGTH_RegisterLayout, U32>;

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
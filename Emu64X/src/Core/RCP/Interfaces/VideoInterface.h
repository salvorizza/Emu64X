#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	struct VI_CTRL_RegisterLayout {
		enum class Field { 
			TYPE, GAMMA_DITHER_ENABLE, GAMMA_ENABLE, DIVOT_ENABLE, VBUS_CLOCK_ENABLE,
			SERRATE, TEST_MODE, AA_MODE, KILL_WE, PIXEL_ADVANCE, DEDITHER_ENABLE
		};

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::TYPE: return { 0, 1 };
			case Field::GAMMA_DITHER_ENABLE: return { 2, 2 };
			case Field::GAMMA_ENABLE: return { 3, 3 };
			case Field::DIVOT_ENABLE: return { 4, 4 };
			case Field::VBUS_CLOCK_ENABLE: return { 5, 5 };
			case Field::SERRATE: return { 6, 6 };
			case Field::TEST_MODE: return { 7, 7 };
			case Field::AA_MODE: return { 8, 9 };
			case Field::KILL_WE: return { 11,11 };
			case Field::PIXEL_ADVANCE: return { 12,15 };
			case Field::DEDITHER_ENABLE: return { 16,16 };
			}
		}
	};
	using VI_CTRL_Register = Register<VI_CTRL_RegisterLayout, U32>;

	struct VI_V_CURRENT_RegisterLayout {
		enum class Field {
			V_FIELD, V_CURRENT
		};

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::V_FIELD: return { 0, 0 };
			case Field::V_CURRENT: return { 1, 9 };
			}
		}
	};
	using VI_V_CURRENT_Register = Register<VI_V_CURRENT_RegisterLayout, U32>;

	struct VI_H_VIDEO_RegisterLayout {
		enum class Field {
			H_END, H_START
		};

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::H_END: return { 0, 9 };
			case Field::H_START: return { 16, 25 };
			}
		}
	};
	using VI_H_VIDEO_Register = Register<VI_H_VIDEO_RegisterLayout, U32>;

	struct VI_V_INTR_RegisterLayout {
		enum class Field {
			V_INTR
		};

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::V_INTR: return { 0, 9 };
			}
		}
	};
	using VI_V_INTR_Register = Register<VI_V_INTR_RegisterLayout, U32>;

	class RCP;

	class VideoInterface {
	public:
		VideoInterface(RCP* rcp);
		~VideoInterface();

		void init();
		void clock(U64 clocks);

		void store(U32 address, U32 value);
		U32 load(U32 address);

		void reset();
	private:
		StringView mName = "VideoInterface";

		RCP* mRCP;

		VI_CTRL_Register VI_CTRL;
		VI_V_INTR_Register VI_V_INTR;
		VI_V_CURRENT_Register VI_V_CURRENT;
		VI_H_VIDEO_Register VI_H_VIDEO;
	};

}
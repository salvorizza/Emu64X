#pragma once

#define VI_CTRL_FIELDS(M) \
    M(TYPE,0,1) M(GAMMA_DITHER_ENABLE,2,2) M(GAMMA_ENABLE,3,3) M(DIVOT_ENABLE,4,4) \
    M(VBUS_CLOCK_ENABLE,5,5) M(SERRATE,6,6) M(TEST_MODE,7,7) M(AA_MODE,8,9) \
    M(KILL_WE,11,11) M(PIXEL_ADVANCE,12,15) M(DEDITHER_ENABLE,16,16)

#define VI_V_CURRENT_FIELDS(M) M(V_FIELD,0,0) M(V_CURRENT,1,9)

#define VI_H_VIDEO_FIELDS(M) M(H_END,0,9) M(H_START,16,25)

#define VI_V_INTR_FIELDS(M) M(V_INTR,0,9)

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {


	DEFINE_REGISTER_LAYOUT(VI_CTRL_Register, U32, VI_CTRL_FIELDS)
	DEFINE_REGISTER_LAYOUT(VI_V_CURRENT_Register, U32, VI_V_CURRENT_FIELDS)
	DEFINE_REGISTER_LAYOUT(VI_H_VIDEO_Register, U32, VI_H_VIDEO_FIELDS)
	DEFINE_REGISTER_LAYOUT(VI_V_INTR_Register, U32, VI_V_INTR_FIELDS)


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
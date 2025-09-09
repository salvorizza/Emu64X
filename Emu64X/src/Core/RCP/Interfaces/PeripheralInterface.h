#pragma once

#define PI_STATUS_FIELDS(M)       \
    M(DMA_BUSY,0,0)               \
    M(IO_BUSY,1,1)                \
    M(DMA_ERROR,2,2)              \
    M(DMA_COMPLETED,3,3)

#define PI_STATUS_WRITE_FIELDS(M) \
    M(RESET_DMA,0,0)              \
    M(CLEAR_INTERRUPT,1,1)

#define PI_BSD_DOM_LAT(M) M(LAT, 0, 7)
#define PI_BSD_DOM_PWD(M) M(PWD, 0, 7)
#define PI_BSD_DOM_PGS(M) M(PGS, 0, 3)
#define PI_BSD_DOM_RLS(M) M(RLS, 0, 1)

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	DEFINE_REGISTER_LAYOUT(PI_STATUS_Register, U32, PI_STATUS_FIELDS)
	DEFINE_REGISTER_LAYOUT(PI_STATUS_Write_Register, U32, PI_STATUS_WRITE_FIELDS)
	DEFINE_REGISTER_LAYOUT(PI_BSD_DOM_LAT_Register, U32, PI_BSD_DOM_LAT)
	DEFINE_REGISTER_LAYOUT(PI_BSD_DOM_PWD_Register, U32, PI_BSD_DOM_PWD)
	DEFINE_REGISTER_LAYOUT(PI_BSD_DOM_PGS_Register, U32, PI_BSD_DOM_PGS)
	DEFINE_REGISTER_LAYOUT(PI_BSD_DOM_RLS_Register, U32, PI_BSD_DOM_RLS)

	class RCP;

	class PeripheralInterface {
	public:
		PeripheralInterface(RCP* rcp);
		~PeripheralInterface();

		void init();
		void clock(U64 clocks);

		void store(U32 address, U32 value);
		U32 load(U32 address);

		void reset();
	private:
		StringView mName = "PeripheralInterface";

		RCP* mRCP;

		PI_STATUS_Register PI_STATUS;
		PI_BSD_DOM_LAT_Register PI_BSD_DOM1_LAT, PI_BSD_DOM2_LAT;
		PI_BSD_DOM_PWD_Register PI_BSD_DOM1_PWD, PI_BSD_DOM2_PWD;
		PI_BSD_DOM_PGS_Register PI_BSD_DOM1_PGS, PI_BSD_DOM2_PGS;
		PI_BSD_DOM_RLS_Register PI_BSD_DOM1_RLS, PI_BSD_DOM2_RLS;
	};

}
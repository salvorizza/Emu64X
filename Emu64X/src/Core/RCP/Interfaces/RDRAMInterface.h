#pragma once

#include "Base/Base.h"

#define RI_MODE_FIELDS(M) M(OP_MODE, 0, 1)  M(STOP_T, 2, 2)  M(STOP_R, 3, 3)
#define RI_CONFIG_FIELDS(M) M(CC, 0, 5)  M(AutoCC, 6, 6)
#define RI_CURRENT_LOAD_FIELDS(M) M(Ack, 0, 0)  M(STOP_R, 3, 3) M(TSEL, 4, 4)
#define RI_SELECT_FIELDS(M) M(RSEL, 0, 3)  M(TSEL, 4, 7)
#define RI_REFRESH_FIELDS(M) M(CleanRefreshDelay, 0, 7) M(DirtyRefreshDelay, 8, 15) M(Bank, 16, 16) M(En, 17, 17) M(Opt, 18, 18) M(MultiBank, 19, 22)
#define RI_LATENCY_FIELDS(M) M(DmaLatencyOverlap, 0, 4)
#define RI_ERROR_FIELDS(M) M(Ack, 0, 0)  M(NAck, 1, 1)  M(Over, 2, 2)
#define RI_BANK_STATUS_FIELDS(M) M(BankValidBits, 0, 7)  M(BankDirtyBits, 8, 15)

#include "Base/Bus.h"

namespace esx {

	DEFINE_REGISTER_LAYOUT(RI_MODE_Register, U32, RI_MODE_FIELDS)
	DEFINE_REGISTER_LAYOUT(RI_CONFIG_Register, U32, RI_CONFIG_FIELDS)
	DEFINE_REGISTER_LAYOUT(RI_CURRENT_LOAD_Register, U32, RI_CURRENT_LOAD_FIELDS)
	DEFINE_REGISTER_LAYOUT(RI_SELECT_Register, U32, RI_SELECT_FIELDS)
	DEFINE_REGISTER_LAYOUT(RI_REFRESH_Register, U32, RI_REFRESH_FIELDS)
	DEFINE_REGISTER_LAYOUT(RI_LATENCY_Register, U32, RI_LATENCY_FIELDS)
	DEFINE_REGISTER_LAYOUT(RI_ERROR_Register, U32, RI_ERROR_FIELDS)
	DEFINE_REGISTER_LAYOUT(RI_BANK_STATUS_Register, U32, RI_BANK_STATUS_FIELDS)

	class RCP;

	class RDRAMInterface {
	public:
		RDRAMInterface(RCP* rcp);
		~RDRAMInterface();

		void init();

		void clock(U64 clocks);

		void store(U32 address, U32 value);
		U32 load(U32 address);

		void reset();
	private:
		StringView mName = "RDRAMInterface";

		RCP* mRCP;

		RI_MODE_Register RI_MODE;
		RI_CONFIG_Register RI_CONFIG;
		RI_SELECT_Register RI_SELECT;
		RI_REFRESH_Register RI_REFRESH;
		RI_LATENCY_Register RI_LATENCY;
		RI_ERROR_Register RI_ERROR;
		RI_BANK_STATUS_Register RI_BANK_STATUS;
	};

}
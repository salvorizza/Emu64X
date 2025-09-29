#pragma once

#include "Base/Base.h"

#define SI_DRAM_ADDR_FIELDS(M) M(DRAM_ADDR, 0, 23)
#define SI_STATUS_FIELDS(M) M(DMA_BUSY, 0, 0)  M(IO_BUSY, 1, 1)  M(READ_PENDING, 2, 2)  M(DMA_ERROR, 3, 3)  M(PCH_STATE, 4, 7)  M(DMA_STATE, 8, 11)  M(INTERRUPT, 12, 12)
#define SI_PIF_AD_64(M) M(PIF_ADDR, 2, 10)

#include "Base/Bus.h"

namespace esx {

	DEFINE_REGISTER_LAYOUT(SI_DRAM_ADDR_Register, U32, SI_DRAM_ADDR_FIELDS)
	DEFINE_REGISTER_LAYOUT(SI_STATUS_Register, U32, SI_STATUS_FIELDS)
	DEFINE_REGISTER_LAYOUT(SI_PIF_AD_64_Register, U32, SI_PIF_AD_64)

	class RCP;
	class SIExternalBus;
	class RDRAM;

	class SerialInterface {
	public:
		SerialInterface(RCP* rcp);
		~SerialInterface();

		void init();

		void clock(U64 clocks);

		void store(U32 address, U32 value);
		U32 load(U32 address);

		void reset();
	private:
		StringView mName = "SerialInterface";

		RCP* mRCP;
		SharedPtr<RDRAM> mRDRAM;
		SharedPtr<SIExternalBus> mSIExtBus;

		SI_DRAM_ADDR_Register SI_DRAM_ADDR;
		SI_STATUS_Register SI_STATUS;
		SI_PIF_AD_64_Register SI_PIF_AD_RD64B;
		SI_PIF_AD_64_Register SI_PIF_AD_WR64B;
	};

}
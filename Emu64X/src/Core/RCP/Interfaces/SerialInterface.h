#pragma once

#include "Base/Base.h"

#define SI_STATUS_FIELDS(M) M(DMA_BUSY, 0, 0)  M(IO_BUSY, 1, 1)  M(READ_PENDING, 2, 2)  M(DMA_ERROR, 3, 3)  M(PCH_STATE, 4, 7)  M(DMA_STATE, 8, 11)  M(INTERRUPT, 12, 12)

#include "Base/Bus.h"

namespace esx {

	DEFINE_REGISTER_LAYOUT(SI_STATUS_Register, U32, SI_STATUS_FIELDS)

	class RCP;

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
		SI_STATUS_Register SI_STATUS;
	};

}
#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

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
	};

}
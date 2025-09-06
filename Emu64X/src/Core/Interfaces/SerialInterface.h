#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	class SerialInterface : public BusDevice {
	public:
		SerialInterface();
		~SerialInterface();

		virtual void clock(U64 clocks) override;

		virtual void store(const StringView& busName, U32 address, U32 value) override;
		virtual void load(const StringView& busName, U32 address, U32& output) override;

		virtual void reset() override;
	private:
	};

}
#pragma once

#include <string_view>
#include <vector>
#include <fstream>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {


	class PIExternalBus : public BusDevice {
		friend class PeripheralInterface;
	public:
		PIExternalBus();
		~PIExternalBus();

		void load(const StringView& busName, U32 address, U32& output) override;

		void reset() override;

		void loadGame(StringView path);
		String getGameCode();
	private:
		FILE* mCartridge;
		U32 mFileSize;
	};

}
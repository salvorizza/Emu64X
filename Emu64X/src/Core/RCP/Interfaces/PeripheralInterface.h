#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	struct PI_STATUS_RegisterLayout {
		enum class Field { DMA_BUSY, IO_BUSY, DMA_ERROR, DMA_COMPLETED };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::DMA_BUSY: return { 0, 0 };
			case Field::IO_BUSY: return { 1, 1 };
			case Field::DMA_ERROR: return { 2, 2 };
			case Field::DMA_COMPLETED: return { 3, 3 };
			}
		}
	};
	using PI_STATUS_Register = Register<PI_STATUS_RegisterLayout, U32>;

	struct PI_STATUS_Write_RegisterLayout {
		enum class Field { RESET_DMA, CLEAR_INTERRUPT };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::RESET_DMA: return { 0, 0 };
			case Field::CLEAR_INTERRUPT: return { 1, 1 };
			}
		}
	};
	using PI_STATUS_Write_Register = Register<PI_STATUS_Write_RegisterLayout, U32>;

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

		PI_STATUS_Register mPI_STATUS;
	};

}
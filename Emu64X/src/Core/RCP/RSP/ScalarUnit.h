#pragma once

#include "Base/Base.h"

#include "Core/MIPS/Common/Coprocessor.h"

namespace esx {

	class R4000;
	class RSP;

	enum class ScalarUnitRegisterType : U8 {
		c0 = 0,
		c1 = 1,
		c2 = 2,
		c3 = 3,
		c4 = 4,
		c5 = 5,
		c6 = 6,
		c7 = 7
	};

	struct SP_DMA_SPADDR_RegisterLayout {
		enum class Field { MEM_ADDR, MEM_BANK };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::MEM_ADDR: return { 0, 11 };
			case Field::MEM_BANK: return { 12, 12 };
			}
		}
	};
	using SP_DMA_SPADDR_Register = Register<SP_DMA_SPADDR_RegisterLayout, U32>;

	struct SP_DMA_RAMADDR_RegisterLayout {
		enum class Field { DRAM_ADDR };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::DRAM_ADDR: return { 0, 23 };
			}
		}
	};
	using SP_DMA_RAMADDR_Register = Register<SP_DMA_RAMADDR_RegisterLayout, U32>;

	struct SP_DMA_RDLEN_RegisterLayout {
		enum class Field { RDLEN, COUNT, SKIP };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::RDLEN: return { 0, 11 };
			case Field::COUNT: return { 12, 19 };
			case Field::SKIP: return { 20, 31 };
			}
		}
	};
	using SP_DMA_RDLEN_Register = Register<SP_DMA_RDLEN_RegisterLayout, U32>;

	struct SP_DMA_WRLEN_RegisterLayout {
		enum class Field { WRLEN, COUNT, SKIP };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::WRLEN: return { 0, 11 };
			case Field::COUNT: return { 12, 19 };
			case Field::SKIP: return { 20, 31 };
			}
		}
	};
	using SP_DMA_WRLEN_Register = Register<SP_DMA_WRLEN_RegisterLayout, U32>;

	struct SP_STATUS_RegisterLayout {
		enum class Field { HALTED, BROKE, DMA_BUSY, DMA_FULL, IO_BUSY, SSTEP, INTBREAK, SIG };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::HALTED: return { 0, 0 };
			case Field::BROKE: return { 1, 1 };
			case Field::DMA_BUSY: return { 2, 2 };
			case Field::DMA_FULL: return { 3, 3 };
			case Field::IO_BUSY: return { 4, 4 };
			case Field::SSTEP: return { 5, 5 };
			case Field::INTBREAK: return { 6, 6 };
			case Field::SIG: return { 7, 14 };
			}
		}
	};
	using SP_STATUS_Register = Register<SP_STATUS_RegisterLayout, U32>;

	struct SP_SEMAPHORE_RegisterLayout {
		enum class Field { SEMAPHORE };

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
			case Field::SEMAPHORE: return { 0, 0 };
			}
		}
	};
	using SP_SEMAPHORE_Register = Register<SP_SEMAPHORE_RegisterLayout, U32>;

	struct SP_STATUS_Write_RegisterLayout {
		enum class Field { 
			CLR_HALT, SET_HALT, CLR_BROKE, CLR_INTR, SET_INTR, CLR_SSTEP, SET_SSTEP, CLR_INTBREAK,
			SET_INTBREAK, CLR_SET_SIG
		};

		static constexpr U32 Mask = 0xFFFFFFFF;

		static constexpr Pair<I32, I32> info(Field f) {
			switch (f) {
				case Field::CLR_HALT:		return { 0, 0 };
				case Field::SET_HALT:		return { 1, 1 };
				case Field::CLR_BROKE:		return { 2, 2 };
				case Field::CLR_INTR:		return { 3, 3 };
				case Field::SET_INTR:		return { 4, 4 };
				case Field::CLR_SSTEP:		return { 5, 5 };
				case Field::SET_SSTEP:		return { 6, 6 };
				case Field::CLR_INTBREAK:	return { 7, 7 };
				case Field::SET_INTBREAK:	return { 8, 8 };
				case Field::CLR_SET_SIG:	return { 9, 24 };
			}
		}
	};
	using SP_STATUS_Write_Register = Register<SP_STATUS_Write_RegisterLayout, U32>;


	class ScalarUnit : public Coprocessor<R4000> {
	public:
		friend class RSP;

		ScalarUnit(R4000* cpu);
		~ScalarUnit() = default;

		void clock(U64 clocks) override;

		void CO() override;

		void unusable() override;
		void reserved() override;
		void signalBreak() override;

		virtual U64 getRegister(RegisterIndex reg) override;
		virtual void setRegister(RegisterIndex reg, U64 value) override;
	private:
		SP_DMA_SPADDR_Register SP_DMA_SPADDR;
		SP_DMA_RAMADDR_Register SP_DMA_RAMADDR;
		SP_DMA_RDLEN_Register SP_DMA_RDLEN;
		SP_DMA_WRLEN_Register SP_DMA_WRLEN;
		SP_STATUS_Register SP_STATUS;
		SP_SEMAPHORE_Register SP_SEMAPHORE;
	};
}
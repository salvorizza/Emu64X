#pragma once

#define SP_DMA_SPADDR_FIELDS(M) M(MEM_ADDR,0,11) M(MEM_BANK,12,12)
#define SP_DMA_RAMADDR_FIELDS(M) M(DRAM_ADDR,0,23)
#define SP_DMA_RDLEN_FIELDS(M) M(RDLEN,0,11) M(COUNT,12,19) M(SKIP,20,31)
#define SP_DMA_WRLEN_FIELDS(M) M(WRLEN,0,11) M(COUNT,12,19) M(SKIP,20,31)
#define SP_STATUS_FIELDS(M) M(HALTED,0,0) M(BROKE,1,1) M(DMA_BUSY,2,2) M(DMA_FULL,3,3) \
                           M(IO_BUSY,4,4) M(SSTEP,5,5) M(INTBREAK,6,6) M(SIG,7,14)
#define SP_SEMAPHORE_FIELDS(M) M(SEMAPHORE,0,0)
#define SP_STATUS_WRITE_FIELDS(M) \
    M(CLR_HALT,0,0) M(SET_HALT,1,1) M(CLR_BROKE,2,2) M(CLR_INTR,3,3) M(SET_INTR,4,4) \
    M(CLR_SSTEP,5,5) M(SET_SSTEP,6,6) M(CLR_INTBREAK,7,7) M(SET_INTBREAK,8,8) M(CLR_SET_SIG,9,24)

#include "Base/Base.h"

#include "Core/MIPS/Common/Coprocessor.h"

namespace esx {

	class R4000;
	class RSP;

	DEFINE_REGISTER_LAYOUT(SP_DMA_SPADDR_Register, U32, SP_DMA_SPADDR_FIELDS)
	DEFINE_REGISTER_LAYOUT(SP_DMA_RAMADDR_Register, U32, SP_DMA_RAMADDR_FIELDS)
	DEFINE_REGISTER_LAYOUT(SP_DMA_RDLEN_Register, U32, SP_DMA_RDLEN_FIELDS)
	DEFINE_REGISTER_LAYOUT(SP_DMA_WRLEN_Register, U32, SP_DMA_WRLEN_FIELDS)
	DEFINE_REGISTER_LAYOUT(SP_STATUS_Register, U32, SP_STATUS_FIELDS)
	DEFINE_REGISTER_LAYOUT(SP_SEMAPHORE_Register, U32, SP_SEMAPHORE_FIELDS)
	DEFINE_REGISTER_LAYOUT(SP_STATUS_Write_Register, U32, SP_STATUS_WRITE_FIELDS)

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
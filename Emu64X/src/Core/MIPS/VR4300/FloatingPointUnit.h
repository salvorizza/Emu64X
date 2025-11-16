#pragma once

#include "Base/Base.h"

#define FCR0_FIELDS(M)  M(Rev, 0, 7)  M(Imp, 8, 15)
#define FCR31_FIELDS(M)  M(RM, 0, 1)  M(Flags, 2, 6)  M(Enables, 7, 11)  M(Cause, 12, 17)  M(C, 23, 23)  M(FS, 24, 24)

#include "Base/Bus.h"

#include "../Common/Coprocessor.h"

namespace esx {

	class VR4300;

	DEFINE_REGISTER_LAYOUT(FCR0Register, U32, FCR0_FIELDS)
	DEFINE_REGISTER_LAYOUT(FCR31Register, U32, FCR31_FIELDS)

	enum class FormatSpec {
		S = 16,
		D = 17,
		W = 20,
		L = 21,
		Reserved
	};

	enum class FPUException {
		I,
		U,
		O,
		Z,
		V,
		E
	};


	class FloatingPointUnit : public Coprocessor<VR4300> {
		friend class CPUStatusPanel;
	public:
		FloatingPointUnit(VR4300* cpu);
		~FloatingPointUnit() = default;

		void clock(U64 clocks) override;

		void CF() override;
		void CT() override;
		void CO() override;

		void ADD();
		void SUB();
		void MUL();
		void DIV();
		void SQRT();
		void ABS();
		void MOV();
		void NEG();
		void ROUNDL();
		void TRUNCL();
		void CEILL();
		void FLOORL();
		void ROUNDW();
		void TRUNCW();
		void CEILW();
		void FLOORW();
		void CVTS();
		void CVTD();
		void CVTW();
		void CVTL();
		void C();

		void unusable() override;
		void reserved() override;
		void signal(FPUException exception);
		BIT checkExceptions();

		U64 getRegister(RegisterIndex reg) override;
		void setRegister(RegisterIndex reg, U64 value) override;
		BIT COC() override;

		static BIT isFloatingPointFormat(FormatSpec fmt) {
			return fmt == FormatSpec::S || fmt == FormatSpec::D;
		}

		static BIT isFixedPointFormat(FormatSpec fmt) {
			return fmt == FormatSpec::W || fmt == FormatSpec::L;
		}

		template<typename T>
		T ValueFPR(RegisterIndex reg, FormatSpec fmt) {
			U64 data = getRegister(reg);

			switch (fmt) {
				case FormatSpec::S: return std::bit_cast<F32>(static_cast<U32>(data & 0xFFFFFFFF));
				case FormatSpec::D: return std::bit_cast<F64>(data);
				case FormatSpec::W: return std::bit_cast<U32>(static_cast<U32>(data & 0xFFFFFFFF));
				case FormatSpec::L: return std::bit_cast<U64>(data);
			}
		}

		template<typename T, typename T2>
		T ConvertFmt(T2 value, FormatSpec from, FormatSpec to) {
			T result = 0;

			if (isFloatingPointFormat(from) && isFixedPointFormat(to)) {
				U64 conversion = static_cast<U64>(value);
				if (((conversion >> 53) & 0x3FF) != 0) signal(FPUException::E);
			} else if (isFixedPointFormat(from) && isFloatingPointFormat(to)) {
				if (((static_cast<U64>(value) >> 55) & 0x7F) != 0) signal(FPUException::E);
			}

			switch (to) {
				case FormatSpec::S: result = static_cast<F32>(value); break;
				case FormatSpec::D: result = static_cast<F64>(value); break;
				case FormatSpec::W: result = static_cast<U32>(value); break;
				case FormatSpec::L: result = static_cast<U64>(value); break;
			}

			return result;
		}

		template<typename T>
		void StoreFPR(RegisterIndex reg, FormatSpec fmt, T value) {
			U64 result;

			if constexpr (sizeof(T) == 4) {
				U32 bits = std::bit_cast<U32>(value);
				result = static_cast<U64>(bits);
			}
			else if constexpr (sizeof(T) == 8) {
				result = std::bit_cast<U64>(value);
			}
			else {
				static_assert(sizeof(T) == 4 || sizeof(T) == 8,
					"StoreFPR only supports 4 or 8 byte types");
			}

			setRegister(reg, result);
		}
	private:
		FCR0Register FCR0;
		FCR31Register FCR31;
		Array<U64, 32> FGR;
	};
}
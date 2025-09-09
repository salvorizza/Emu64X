#pragma once

#include <UI/Panels/Panel.h>
#include <Core/MIPS/VR4300/VR4300.h>
#include <Core/RDRAM.h>

#include <map>
#include <vector>

namespace esx {

	enum class DebugState {
		None,
		Idle,
		Start,
		Running,
		Breakpoint,
		Step,
		StepOver,
		Stop
	};

	class DisassemblerPanel : public Panel {
	public:
		DisassemblerPanel();
		~DisassemblerPanel();

		void setInstance(const SharedPtr<VR4300>& pInstance) { mInstance = pInstance;}
		void setBus(const SharedPtr<Bus>& pBus) { mBus = pBus; }

		bool breakFunction(U32 address);

		void onUpdate();

		void onPlay();
		void onPause();
		void onStepForward();
		void onStepOver();

		DebugState getDebugState() const { return mDebugState; }

		void loadEXE(const std::filesystem::path& exePath);

	protected:
		virtual void onImGuiRender() override;

	private:
		struct Instruction {
			U32 Address;
			std::string Mnemonic;
		};

		struct Breakpoint {
			bool Enabled = true;
			U32 Address;
			U32 PhysAddress;
		};

	private:
		void disassemble(uint32_t startAddress, size_t size);

		void setDebugState(DebugState debugState) { mPrevDebugState = mDebugState; mDebugState = debugState; }

		SharedPtr<VR4300> mInstance;
		SharedPtr<Bus> mBus;

		std::vector<Instruction> mInstructions;
		std::vector<Breakpoint> mBreakpoints;

		DebugState mDebugState;
		DebugState mPrevDebugState;
		bool mScrollToCurrent;
		uint32_t mCurrent;
		U32 mNextPC;

		static const size_t disassembleRange = 10;


	};

}

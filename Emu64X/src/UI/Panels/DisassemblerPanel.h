#pragma once

#include <UI/Panels/Panel.h>
#include <Core/MIPS/VR4300/VR4300.h>
#include <Core/MIPS/R4000/R4000.h>
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

	enum class DisassemblerProc : U8 {
		VR4300, RSP
	};

	class DisassemblerPanel : public Panel {
	public:
		struct Instruction {
			U32 Address;
			std::string Mnemonic;
		};

		struct Breakpoint {
			bool Enabled = true;
			U32 Address;
			U32 PhysAddress;
		};

		struct DisassemblerState {
			std::function<std::pair<U32, U32>()> mAdressingFunc;
			std::function<Instruction(U32*)> mDecodeFunc;
			std::vector<Instruction> mInstructions;
			std::vector<Breakpoint> mBreakpoints;

			bool mScrollToCurrent = false;
			uint32_t mCurrent;
			U32 mNextPC;
		};
	public:
		DisassemblerPanel();
		~DisassemblerPanel();

		void setInstance(const SharedPtr<VR4300>& pInstance) { mInstance = pInstance;}
		void setInstance(const SharedPtr<R4000>& pInstance) { mInstanceR4000 = pInstance; }
		void setBus(const SharedPtr<Bus>& pBus) { mBus = pBus; }

		bool breakFunction();

		void onUpdate();

		void onPlay();
		void onPause();
		void onStepForward();
		void onStepOver();

		DebugState getDebugState() const { return mDebugState; }

		void loadEXE(const std::filesystem::path& exePath);

		void setAddressingFunc(DisassemblerProc disassemblerProc, const std::function<std::pair<U32, U32>()> func) { mDisassemblerStates.at(disassemblerProc).mAdressingFunc = func; }
		void setDecodeFunc(DisassemblerProc disassemblerProc, const std::function<Instruction(U32*)> func) { mDisassemblerStates.at(disassemblerProc).mDecodeFunc = func; }

	protected:
		virtual void onImGuiRender() override;

	private:
		DisassemblerState& getDisassemblerState() { return mDisassemblerStates.at(mCurrentDisassemblerProc); }
		const DisassemblerState& getDisassemblerState() const { return mDisassemblerStates.at(mCurrentDisassemblerProc); }
		U32 getTrueCurrentInstancePC() const { return Bus::toPhysicalAddress(mCurrentDisassemblerProc == DisassemblerProc::VR4300 ? mInstance->mPC : mInstanceR4000->mPC); }
		U32 getCurrentInstancePC() const { return Bus::toPhysicalAddress(mCurrentDisassemblerProc == DisassemblerProc::VR4300 ? mInstance->mPC : (0x04001000 + mInstanceR4000->mPC)); }
		U32 getCurrentInstanceNextPC() const { return Bus::toPhysicalAddress(mCurrentDisassemblerProc == DisassemblerProc::VR4300 ? mInstance->mNextPC : (0x04001000 + mInstanceR4000->mNextPC)); }

		void setDebugState(DebugState debugState) { mPrevDebugState = mDebugState; mDebugState = debugState; }

		SharedPtr<VR4300> mInstance;
		SharedPtr<R4000> mInstanceR4000;
		SharedPtr<Bus> mBus;
		DisassemblerProc mCurrentDisassemblerProc = DisassemblerProc::VR4300;
		UnorderedMap<DisassemblerProc, DisassemblerState> mDisassemblerStates;
		DebugState mDebugState = DebugState::Idle;
		DebugState mPrevDebugState = DebugState::None;

		static const size_t disassembleRange = 10;
	};

}

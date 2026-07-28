#include "UI/Panels/DisassemblerPanel.h"

#include "UI/Window/FontAwesome5.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include <imgui.h>

#include "Core/Scheduler.h"

namespace esx {



	DisassemblerPanel::DisassemblerPanel()
		:	Panel("Disassembler", false),
			mInstance(nullptr)
	{
		mDisassemblerStates[DisassemblerProc::VR4300] = {};
		mDisassemblerStates[DisassemblerProc::RSP] = {};
	}

	DisassemblerPanel::~DisassemblerPanel()
	{
	}

	bool DisassemblerPanel::breakFunction()
	{
		switch (mDebugState) {
			case DebugState::Running:
				for (auto& [k,state] : mDisassemblerStates) {
					if (state.mBreakpoints.size() > 0) {
						auto it = std::find_if(state.mBreakpoints.begin(), state.mBreakpoints.end(), [&](Breakpoint& b) { return Bus::toPhysicalAddress(b.PhysAddress) == getCurrentInstancePC() && b.Enabled; });
						return it != state.mBreakpoints.end();
					}
				}
				return false;

			case DebugState::Step:
			case DebugState::StepOver:
				return getCurrentInstancePC() == getDisassemblerState().mNextPC;
			default:
				break;
		}

		return false;
	}


	void DisassemblerPanel::onUpdate()
	{
		auto& state = getDisassemblerState();

		switch (mDebugState) {
			case DebugState::Start:
				setDebugState(DebugState::Running);
				break;

			case DebugState::StepOver:
			case DebugState::Step:
			case DebugState::Running: {
				//U64 clockStart = mInstance->getClocks();

				BIT newFrameAvailable = ESX_FALSE;
				while (newFrameAvailable == ESX_FALSE) {
					while (Scheduler::HasEvents() == ESX_FALSE || mInstance->getClocks() < Scheduler::NextEvent().ClockTarget) {
						if (breakFunction()) {
							state.mScrollToCurrent = true;
							state.mCurrent = getTrueCurrentInstancePC();
							state.mNextPC = getCurrentInstanceNextPC();
							setDebugState(DebugState::Breakpoint);
							break;
						}

						mInstance->clock();

						if (mDebugState == DebugState::Breakpoint) {
							break;
						}
					}


					if (mDebugState == DebugState::Breakpoint) {
						break;
					}

					if (mInstance->getClocks() >= Scheduler::NextEvent().ClockTarget) {
						if (Scheduler::NextEvent().Type == SchedulerEventType::GPUFrameStart) {
							newFrameAvailable = ESX_TRUE;
						}

						Scheduler::ExecuteEvent();
						Scheduler::Progress();
					}
				}
				/*U64 clockEnd = mInstance->getClocks();
				ESX_CORE_LOG_TRACE("Emu FPS: {}", 93750000.0f / (clockEnd - clockStart));*/
				break;
			}

			case DebugState::Stop:
				setDebugState(DebugState::Idle);
				break;
			default:
				break;
		}
	}

	void DisassemblerPanel::loadEXE(const std::filesystem::path& exePath)
	{
	}

	void DisassemblerPanel::onImGuiRender()
	{

		if (ImGui::BeginTabBar("SelectProcessor"))
		{
			if (ImGui::BeginTabItem("VR4300")) {
				mCurrentDisassemblerProc = DisassemblerProc::VR4300;
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("RSP")) {
				mCurrentDisassemblerProc = DisassemblerProc::RSP;
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();

		auto& state = getDisassemblerState();

		float availWidth = ImGui::GetContentRegionAvail().x;
		float oneCharSize = ImGui::CalcTextSize("A").x;
		float bulletSize = oneCharSize * 3;
		float addressingSize = oneCharSize * 11;
		float contentCellsWidth = availWidth - addressingSize - bulletSize;

		if (mDebugState == DebugState::Idle || mDebugState == DebugState::Breakpoint) {
			if (ImGui::Button(ICON_FA_PLAY))
				onPlay();
		}
		else {
			if (ImGui::Button(ICON_FA_PAUSE)) {
				onPause();
			}
		}
		if (mDebugState == DebugState::Breakpoint) {
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_STEP_FORWARD)) onStepForward();
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_ARROWS_TURN_DOWN)) onStepOver();
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_GOLF_BALL)) {
				state.mScrollToCurrent = true;
				state.mCurrent = getTrueCurrentInstancePC();
			}
		}
		
		switch (mDebugState)
		{
			case DebugState::Idle:
				ImGui::TextUnformatted("State: Idle");
				break;
			case DebugState::Start:
				ImGui::TextUnformatted("State: Start");
				break;
			case DebugState::Running:
				ImGui::TextUnformatted("State: Running");
				break;
			case DebugState::Breakpoint:
				ImGui::TextUnformatted("State: Breakpoint");
				break;
			case DebugState::Step:
				ImGui::TextUnformatted("State: Step");
				break;
			case DebugState::Stop:
				ImGui::TextUnformatted("State: Stop");
				break;
			default:
				break;
		}

		auto [baseAddress, adressingSize] = getDisassemblerState().mAdressingFunc();

		float sizeY = ImGui::GetContentRegionAvail().y;


		if (ImGui::BeginChild("##child", ImVec2(0, sizeY * 0.75f))) {
			if (ImGui::BeginTable("Disassembly Table", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, bulletSize);
				ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, addressingSize);
				ImGui::TableSetupColumn("Mnemonic", ImGuiTableColumnFlags_WidthFixed, contentCellsWidth);
				ImGui::TableHeadersRow();

				size_t numInstructions = adressingSize / 4;

				ImGuiListClipper clipper;
				clipper.Begin(numInstructions);
				if (state.mScrollToCurrent) {
					U32 index = 0;
					
					index = (state.mCurrent - Bus::toPhysicalAddress(baseAddress)) / 4;

					clipper.ForceDisplayRangeByIndices(index, index + 1);
				}
				while (clipper.Step())
				{
					for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
						U32 physAddress = baseAddress + row * 4;

						Instruction instruction = state.mDecodeFunc(&physAddress);
						
						U32 address = Bus::toPhysicalAddress(instruction.Address);
						auto breakpointIt = std::find_if(state.mBreakpoints.begin(), state.mBreakpoints.end(), [&](Breakpoint& b) { return b.PhysAddress == physAddress && b.Enabled; });
						BIT breakpointFound = breakpointIt != state.mBreakpoints.end();

						ImGui::TableNextRow();
						if (state.mScrollToCurrent && address == state.mCurrent) {
							ImGui::SetScrollHereY(0.75);
							state.mScrollToCurrent = false;
						}

						if (mDebugState != DebugState::Idle && physAddress == getCurrentInstancePC()) {
							ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(230, 100, 120, 125));
							ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(180, 50, 70, 125));
						}

						ImGui::TableNextColumn();
						ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 0));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 0));
						ImGui::PushStyleColor(ImGuiCol_Text, breakpointFound ? IM_COL32(255, 0, 0, 255) : IM_COL32(255, 255, 255, 255));
						ImGui::PushID(address);
						if (ImGui::Button(ICON_FA_CIRCLE)) {
							if (breakpointFound) {
								state.mBreakpoints.erase(breakpointIt);
							} else {
								Breakpoint breakpoint = {};
								breakpoint.Enabled = ESX_TRUE;
								breakpoint.Address = address;
								breakpoint.PhysAddress = physAddress;
								state.mBreakpoints.push_back(breakpoint);
							}
						}
						ImGui::PopID();
						ImGui::PopStyleColor();
						ImGui::PopStyleColor();
						ImGui::PopStyleColor();
						ImGui::PopStyleColor();

						ImGui::TableNextColumn();
						ImGui::Text("0x%08X", address);

						ImGui::TableNextColumn();
						ImGui::TextUnformatted(instruction.Mnemonic.c_str());
					}
				}
				clipper.End();
				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
		
		if (ImGui::BeginTabBar("SelectDisassembleRom2"))
		{
			if (ImGui::BeginTabItem("Breakpoints")) {
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		if (ImGui::Button(ICON_FA_PLUS)) {
			getDisassemblerState().mBreakpoints.emplace(state.mBreakpoints.begin());
		}

		sizeY = ImGui::GetContentRegionAvail().y;
		contentCellsWidth = availWidth - (oneCharSize * 13);
		if (ImGui::BeginChild("##childbr", ImVec2(0, sizeY))) {
			if (ImGui::BeginTable("Breakpoints Table", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders, ImVec2(0, sizeY))) {
				ImGui::TableSetupColumn("Break", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderLabel, oneCharSize * 3);
				ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, contentCellsWidth);
				ImGui::TableSetupColumn("Delete", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderLabel, oneCharSize * 3);
				ImGui::TableHeadersRow();

				I64 indexToDelete = -1;
				int i = 0;
				static char addressBuffer[32];
				for (auto& breakpoint : state.mBreakpoints) {
					ImGui::TableNextRow();

					ImGui::PushID(i);

					ImGui::TableNextColumn();
					if (breakpoint.Enabled) {
						if (ImGui::Button(ICON_FA_EYE)) breakpoint.Enabled = false;
					}
					else {
						if (ImGui::Button(ICON_FA_EYE_SLASH)) breakpoint.Enabled = true;
					}

					ImGui::TableNextColumn();
					sprintf_s(addressBuffer, 32, "0x%08X", breakpoint.Address);
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0));
					if (ImGui::InputText("##goto", addressBuffer, IM_ARRAYSIZE(addressBuffer), ImGuiInputTextFlags_CharsHexadecimal))
					{
						U32 addr;
						if (sscanf_s(addressBuffer, "0x%08X", &addr) == 1 || sscanf_s(addressBuffer, "%08X", &addr) == 1) {
							breakpoint.Address = addr;
							breakpoint.PhysAddress = mCurrentDisassemblerProc == DisassemblerProc::VR4300 ? Bus::toPhysicalAddress(breakpoint.Address) : (0x04001000 + breakpoint.Address);
						}
					}
					ImGui::PopStyleColor(1);

					ImGui::TableNextColumn();
					if (ImGui::Button(ICON_FA_TRASH)) {
						indexToDelete = i;
					}

					ImGui::PopID();

					i++;
				}


				if (indexToDelete != -1) {
					getDisassemblerState().mBreakpoints.erase(state.mBreakpoints.begin() + indexToDelete);
				}

				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
	}

	void DisassemblerPanel::onPlay() {
		switch (mDebugState) {
			case DebugState::Idle:
				setDebugState(DebugState::Start);
				break;

			case DebugState::Breakpoint:
				mInstance->clock();
			default:
				setDebugState(DebugState::Running);
				break;
		}
	}

	void DisassemblerPanel::onPause() {
		auto& state = getDisassemblerState();

		setDebugState(DebugState::Breakpoint);
		//disassemble(mInstance->mPC - 4 * disassembleRange, 4 * disassembleRange * 2);
		state.mScrollToCurrent = true;
		state.mCurrent = getTrueCurrentInstancePC();
	}

	void DisassemblerPanel::onStepForward() {
		auto& state = getDisassemblerState();

		if (mDebugState == DebugState::Breakpoint) {
			setDebugState(DebugState::Step);
			state.mScrollToCurrent = true;
			state.mCurrent = getTrueCurrentInstancePC();
			state.mNextPC = getCurrentInstanceNextPC();
		}
	}

	void DisassemblerPanel::onStepOver()
	{
		auto& state = getDisassemblerState();

		if (mDebugState == DebugState::Breakpoint) {
			state.mNextPC = getCurrentInstancePC() + 4;
			setDebugState(DebugState::StepOver);
			state.mScrollToCurrent = true;
			state.mCurrent = getTrueCurrentInstancePC();
		}
	}

}
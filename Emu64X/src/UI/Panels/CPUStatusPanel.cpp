#include "UI/Panels/CPUStatusPanel.h"

#include <imgui.h>

#include "Core/RCP/RSP/VectorUnit.h"

namespace esx {



	CPUStatusPanel::CPUStatusPanel()
		:	Panel("CPU Status", false),
			mInstance(nullptr)
	{}

	CPUStatusPanel::~CPUStatusPanel()
	{
	}


	void CPUStatusPanel::onImGuiRender() {
		constexpr static const char* registersMnemonics[] = {
			"$zero",
			"$at",
			"$v0","$v1",
			"$a0","$a1","$a2","$a3",
			"$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7",
			"$s0","$s1","$s2","$s3","$s4","$s5","$s6","$s7",
			"$t8","$t9",
			"$k0","$k1",
			"$gp",
			"$sp",
			"$fp",
			"$ra"
		};

		constexpr static const char* cop0RegistersMnemonics[] = {
			"Index",
			"Random",
			"EntryLo0",
			"EntryLo1",
			"Context",
			"PageMask",
			"Wired",
			"*Garbage*",
			"BadVAddr",
			"Count",
			"EntryHi",
			"Compare",
			"SR",
			"Cause",
			"EPC",
			"PRId",
			"Config",
			"LLAddr",
			"WatchLo",
			"WatchHi",
			"XContext",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"PErr",
			"CacheErr",
			"TagLo",
			"TagHi",
			"ErrorEPC",
			"*Garbage*",
		};

		constexpr static const char* R4000_cop0RegistersMnemonics[] = {
			"SP_DMA_SPADDR",
			"SP_DMA_RAMADDR",
			"SP_DMA_RDLEN",
			"SP_DMA_WRLEN",
			"SP_STATUS",
			"SP_DMA_FULL",
			"SP_DMA_BUSY",
			"SP_SEMAPHORE"
		};

		enum class TabItemProc {
			VR4300, RSP
		};

		enum class TabItem {
			CPU, CP0, CP1, CP2
		};

		static TabItemProc tabItemProc = TabItemProc::VR4300;
		static TabItem tabItem = TabItem::CPU;
		static TabItem R4000tabItem = TabItem::CPU;

		if (ImGui::BeginTabBar("SelectProcessor"))
		{
			if (ImGui::BeginTabItem("VR4300")) {
				tabItemProc = TabItemProc::VR4300;
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("RSP")) {
				tabItemProc = TabItemProc::RSP;
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();

		if (ImGui::BeginTabBar("SelectCoprocessor"))
		{
			if (ImGui::BeginTabItem("CPU")) {
				if (tabItemProc == TabItemProc::VR4300) {
					tabItem = TabItem::CPU;
				} else {
					R4000tabItem = TabItem::CPU;
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("CP0")) {
				if (tabItemProc == TabItemProc::VR4300) {
					tabItem = TabItem::CP0;
				} else {
					R4000tabItem = TabItem::CP0;
				}
				ImGui::EndTabItem();
			}

			if (tabItemProc == TabItemProc::VR4300) {
				if (ImGui::BeginTabItem("CP1")) {
					tabItem = TabItem::CP1;
					ImGui::EndTabItem();
				}
			}

			if (tabItemProc == TabItemProc::RSP) {
				if (ImGui::BeginTabItem("CP2")) {
					R4000tabItem = TabItem::CP2;
					ImGui::EndTabItem();
				}
			}
		}
		ImGui::EndTabBar();

		float availWidth = ImGui::GetContentRegionAvail().x;

		TabItem currentItem = tabItemProc == TabItemProc::VR4300 ? tabItem : R4000tabItem;
		switch (currentItem) {
			case TabItem::CPU: {
				if (ImGui::BeginTable("CPUStatusTable", 3, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Hideable)) {
					ImGui::TableSetupColumn("GPR", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.15f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.50f);
					ImGui::TableHeadersRow();

					for (int i = 0; i < 32; i++) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(registersMnemonics[i]);
						ImGui::TableNextColumn();
						if (tabItemProc == TabItemProc::VR4300) {
							ImGui::Text("0x%016llX", mInstance->mRegisters[i]);
						} else {
							ImGui::Text("0x%08X", mInstanceR4000->mRegisters[i]);
						}
					}

					if (tabItemProc == TabItemProc::VR4300) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("HI");
						ImGui::TableNextColumn();
						ImGui::Text("0x%016llX", mInstance->mHI);

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("LO");
						ImGui::TableNextColumn();
						ImGui::Text("0x%016llX", mInstance->mLO);

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("PC");
						ImGui::TableNextColumn();
						ImGui::Text("0x%016llX", mInstance->mPC);
					} else {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TextUnformatted("PC");
						ImGui::TableNextColumn();
						ImGui::Text("0x%08X", mInstanceR4000->mPC);
					}

				}
				ImGui::EndTable();
				ImGui::Text("Clocks: 0x%016llX", mInstance->getClocks());
				break;
			}

			case TabItem::CP0: {
				if (ImGui::BeginTable("CP0StatusTable", 3, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Hideable)) {
					ImGui::TableSetupColumn("Register", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.35f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.50f);
					ImGui::TableHeadersRow();

					for (int i = 0; i < (tabItemProc == TabItemProc::VR4300 ? 32 : 8); i++) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(tabItemProc == TabItemProc::VR4300 ? cop0RegistersMnemonics[i] : R4000_cop0RegistersMnemonics[i]);
						ImGui::TableNextColumn();
						if (tabItemProc == TabItemProc::VR4300) {
							ImGui::Text("0x%016llX", mInstance->mCP0->getRegister(RegisterIndex(i)));
						} else {
							ImGui::Text("0x%08X", mInstanceR4000->mCOPs[0]->getRegister(RegisterIndex(i)));
						}
					}
					
				}
				ImGui::EndTable();
				break;
			}

			case TabItem::CP1: {
				static int selected_fmt = U8(FormatSpec::Reserved);

				ImGui::RadioButton("raw hex", &selected_fmt, U8(FormatSpec::Reserved));
				ImGui::RadioButton("single", &selected_fmt, U8(FormatSpec::S));
				ImGui::SameLine();
				ImGui::RadioButton("double", &selected_fmt, U8(FormatSpec::D));
				ImGui::RadioButton("word", &selected_fmt, U8(FormatSpec::W));
				ImGui::SameLine();
				ImGui::RadioButton("long", &selected_fmt, U8(FormatSpec::L));

				if (ImGui::BeginTable("CP1StatusTable", 3, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Hideable)) {
					ImGui::TableSetupColumn("Register", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.30f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.50f);
					ImGui::TableHeadersRow();

					for (int i = 0; i < 32; i++) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("FGR%d",i);
						ImGui::TableNextColumn();

						switch (FormatSpec(selected_fmt)) {
							case FormatSpec::S: {
								ImGui::Text("%f", *reinterpret_cast<float*>(&mInstance->mCP1->FGR[i]));
								break;
							}

							case FormatSpec::D: {
								ImGui::Text("%f", *reinterpret_cast<double*>(&mInstance->mCP1->FGR[i]));
								break;
							}

							case FormatSpec::W: {
								ImGui::Text("%d", static_cast<U32>(mInstance->mCP1->FGR[i]));
								break;
							}

							case FormatSpec::L: {
								ImGui::Text("%lld", mInstance->mCP1->FGR[i]);
								break;
							}

							case FormatSpec::Reserved: {
								ImGui::Text("0x%016llX", mInstance->mCP1->FGR[i]);
							}
							

						}
					}

				}
				ImGui::EndTable();
				break;
			}

			case TabItem::CP2: {
				if (ImGui::BeginTable("CP2StatusTable", 3, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Hideable)) {
					ImGui::TableSetupColumn("Register", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.05f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.90f);
					ImGui::TableHeadersRow();

					for (int i = 0; i < 32; i++) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("v%02d", i);
						ImGui::TableNextColumn();
						for (I32 j = 0; j < 8; j++) {
							ImGui::Text("%04X", mInstanceR4000->mRCP->mRSP->mVU->VPR[i][j]);
							ImGui::SameLine();
						}
					}

				}
				ImGui::EndTable();
				break;
			}
		}

	}

}
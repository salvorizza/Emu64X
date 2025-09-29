#include "UI/Panels/CPUStatusPanel.h"

#include <imgui.h>

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

		enum class TabItem {
			CPU,CP0,CP1
		};

		static TabItem tabItem = TabItem::CPU;

		if (ImGui::BeginTabBar("SelectCoprocessor"))
		{
			if (ImGui::BeginTabItem("CPU")) {
				tabItem = TabItem::CPU;
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("CP0")) {
				tabItem = TabItem::CP0;
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("CP1")) {
				tabItem = TabItem::CP1;
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();

		float availWidth = ImGui::GetContentRegionAvail().x;

		switch (tabItem) {
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
						ImGui::Text("0x%016llX", mInstance->mRegisters[i]);
					}

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

				}
				ImGui::EndTable();
				break;
			}

			case TabItem::CP0: {
				if (ImGui::BeginTable("CP0StatusTable", 3, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Hideable)) {
					ImGui::TableSetupColumn("Register", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.30f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.50f);
					ImGui::TableHeadersRow();

					for (int i = 0; i < 32; i++) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(cop0RegistersMnemonics[i]);
						ImGui::TableNextColumn();
						ImGui::Text("0x%016llX", mInstance->mCP0->getRegister(RegisterIndex(i)));
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
		}

	}

}
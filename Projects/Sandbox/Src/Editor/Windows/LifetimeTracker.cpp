#include "PreCompiled.h"
#include "LifetimeTracker.h"

#include "DX12/NewCore/DX12Core3.h"
#include "Utils/Utils.h"

void RSE::LifetimeTracker::Render()
{
	if (!m_Enabled) return;

	if (ImGui::Begin("Lifetime Tracker"))
	{
		if (RS::LaunchArguments::Contains(RS::LaunchParams::logResources))
		{
			static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Sortable;

			ImGui::Text("This does not take into account pending removals!");
			bool open = ImGui::CollapsingHeader("Resources", ImGuiTreeNodeFlags_DefaultOpen);
			if (open && ImGui::BeginTable("Table1", 3, flags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 5)))
			{
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_DefaultSort);
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Alive Time");
				ImGui::TableHeadersRow();
				std::lock_guard<std::mutex> lock(RS::DX12Core3::s_ResourceLifetimeTrackingMutex);
				for (auto& entry : RS::DX12Core3::s_ResourceLifetimeTrackingData)
				{
					ImGui::TableNextColumn(); // ID
					std::string ID = RS::Utils::Format("{}", entry.first);
					ImGui::Text(ID.c_str());

					ImGui::TableNextColumn(); // Name
					ImGui::Text(entry.second.name);

					ImGui::TableNextColumn(); // Alive Time
					{
						ImGui::BeginDisabled();
						ImGui::PushItemWidth(-1);
						ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
						ImGuiDataType dataType = ImGuiDataType_U64;

						const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
						std::string floatLabel = RS::Utils::Format("##Integer-{}", entry.first);
						ImGui::InputScalar(floatLabel.c_str(), dataType, &entry.second.startTime, NULL, NULL, format, flags);
						ImGui::PopItemWidth();
						ImGui::EndDisabled();
					}
				}

				ImGui::EndTable();
			}
			std::array<uint32, FRAME_BUFFER_COUNT> numPendingRemovals = RS::DX12Core3::Get()->GetNumberOfPendingPremovals();
			ImGui::Text("Penging Removals:");
			for (uint32 i = 0; i < FRAME_BUFFER_COUNT; ++i)
			ImGui::Text("\t[%u] -> %u", i, numPendingRemovals[i]);
			ImGui::Text("Global resource states: %u", RS::ResourceStateTracker::GetNumberOfGlobalResources());
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Information not available! (use -logResources)");
		}

		ImGui::End();
	}
}

#include "ConsoleInspectorTool.h"

void RS::ConsoleInspectorTool::ImGuiRender(
	std::vector<RS::Console::Variable>& variables,
	std::vector<RS::Console::Variable>& functions,
	std::vector<RS::Console::Variable>& unknowns)
{
	static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Sortable;

	// TODO: Add search functionality. Make the Search function in Console more generic and use it here too.
	//ImGui::InputText();

	bool open = ImGui::CollapsingHeader("Variables", ImGuiTreeNodeFlags_DefaultOpen);
	if (open && ImGui::BeginTable("Table1", 3, flags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 5)))
	{
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort);
		ImGui::TableSetupColumn("Data");
		ImGui::TableSetupColumn("Access");
		ImGui::TableHeadersRow();
		SortTable(variables);
		for (Console::Variable& var : variables)
			RenderVariable(var);

		ImGui::EndTable();
	}

	open = ImGui::CollapsingHeader("Functions", ImGuiTreeNodeFlags_DefaultOpen);
	if (open && ImGui::BeginTable("Table2", 3, flags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 5)))
	{
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Data");
		ImGui::TableSetupColumn("Access");
		ImGui::TableHeadersRow();
		SortTable(functions);
		for (Console::Variable& var : functions)
			RenderVariable(var);

		ImGui::EndTable();
	}
	open = ImGui::CollapsingHeader("Unknowns", ImGuiTreeNodeFlags_DefaultOpen);
	if (open && ImGui::BeginTable("Table3", 3, flags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 5)))
	{
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Data");
		ImGui::TableSetupColumn("Access");
		ImGui::TableHeadersRow();
		SortTable(unknowns);
		for (Console::Variable& var : unknowns)
			RenderVariable(var);

		ImGui::EndTable();
	}
}

void RS::ConsoleInspectorTool::RenderVariable(Console::Variable& var)
{
	const bool readOnly = (var.flags & Console::Flag::ReadOnly) != 0;

	ImGui::TableNextColumn();
	ImGui::Text(var.name.c_str());

	ImGui::TableNextColumn();
	if (Console::IsTypeVariable(var.type))
	{
		if (Console::IsTypeInt(var.type) || Console::IsTypeUInt(var.type))
		{
			ImGui::BeginDisabled(readOnly);
			RenderIntVariable(var);
			ImGui::EndDisabled();
		}
		else if (Console::IsTypeFloat(var.type))
		{
			ImGui::BeginDisabled(readOnly);
			RenderFloatVariable(var);
			ImGui::EndDisabled();
		}
		else if (Console::IsTypeBool(var.type))
		{
			ImGui::BeginDisabled(readOnly);
			RenderBoolVariable(var);
			ImGui::EndDisabled();
		}
		else
		{
			std::string value = Console::VarValueToString(var.type, var.pVar);
			ImGui::Text(value.c_str());
		}
	}
	else if (Console::IsTypeFunction(var.type))
		ImGui::Text("Function");
	else
		ImGui::Text("Unknown");

	ImGui::TableNextColumn();
	if (readOnly)
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "R");
	else
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "R/W");
}

void RS::ConsoleInspectorTool::RenderFloatVariable(Console::Variable& var)
{
	ImGui::PushItemWidth(-1);
	float v = var.type == Console::Type::Float ? *(float*)var.pVar : (float)*(double*)var.pVar;
	uint32 numDecimals = Utils::GetNumDecimals(v);
	std::string format = std::format("%.{}f", std::max(numDecimals, 1u));
	std::string floatLabel = std::format("##Float-{}", var.name);
	if (ImGui::InputFloat(floatLabel.c_str(), &v, 0.0f, 0.0f, format.c_str(), ImGuiInputTextFlags_None))
	{
		if (var.type == Console::Type::Float) *(float*)var.pVar = v;
		else *(double*)var.pVar = (double)v;
	}
	ImGui::PopItemWidth();
}

void RS::ConsoleInspectorTool::RenderIntVariable(Console::Variable& var)
{
	ImGui::PushItemWidth(-1);
	ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
	ImGuiDataType dataType = ImGuiDataType_S32;
	switch (var.type)
	{
	case Console::Type::Int8: dataType = ImGuiDataType_S8; break;
	case Console::Type::Int16: dataType = ImGuiDataType_S16; break;
	case Console::Type::Int32: dataType = ImGuiDataType_S32; break;
	case Console::Type::Int64: dataType = ImGuiDataType_S64; break;
	case Console::Type::UInt8: dataType = ImGuiDataType_U8; break;
	case Console::Type::UInt16: dataType = ImGuiDataType_U16; break;
	case Console::Type::UInt32: dataType = ImGuiDataType_U32; break;
	case Console::Type::UInt64: dataType = ImGuiDataType_U64; break;
	default: RS_ASSERT(false, "Should not happen!"); break;
	}

	const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
	std::string floatLabel = std::format("##Integer-{}", var.name);
	ImGui::InputScalar(floatLabel.c_str(), dataType, var.pVar, NULL, NULL, format, flags);
	ImGui::PopItemWidth();
}

void RS::ConsoleInspectorTool::RenderBoolVariable(Console::Variable& var)
{
	std::string boolLabel = std::format("##Bool-{}", var.name);
	ImGui::Checkbox(boolLabel.c_str(), (bool*)var.pVar);
}

void RS::ConsoleInspectorTool::SortTable(std::vector<Console::Variable>& refVariables)
{
	static ImGuiTableSortSpecs* s_current_sort_specs = nullptr;

	ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs();
	if (sorts_specs == nullptr) return;
	if (!sorts_specs->SpecsDirty) return;
	if (refVariables.size() <= 1) return;

	s_current_sort_specs = sorts_specs; // Store in variable accessible by the sort function.
	std::sort(refVariables.begin(), refVariables.end(),
		[&](const Console::Variable& a, const Console::Variable& b) -> bool
		{
			for (int i = 0; i < s_current_sort_specs->SpecsCount; ++i)
			{
				const ImGuiTableColumnSortSpecs* sort_spec = &s_current_sort_specs->Specs[i];
				int delta = 0;
				switch (sort_spec->ColumnIndex)
				{
				case 0: delta = strcmp(a.name.c_str(), b.name.c_str()); break;
				case 1: delta = 0; break; // TODO: Add sort for type value!
				case 2: delta = (a.flags & Console::Flag::ReadOnly) - (b.flags & Console::Flag::ReadOnly); break;
				default:
					RS_ASSERT(false);
					break;
				}
				if (delta > 0)
					return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? false : true;
				if (delta < 0)
					return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? true : false;
			}
			// Fallback sort
			return a.name < b.name;
		}
	);

	s_current_sort_specs = nullptr;
	sorts_specs->SpecsDirty = false;
}

void RS::DebugConsoleInspectorWindow::Render()
{
	Console* pConsole = Console::Get();

	uint64 currentConsoleStateHash = pConsole->GetStateHash();
	if (m_StoredConsoleStateHash != currentConsoleStateHash)
	{
		m_Variables = pConsole->GetVariables(Console::s_VariableTypes);
		m_Functions = pConsole->GetVariables({ Console::Type::Function });
		m_Unknowns = pConsole->GetVariables({ Console::Type::Unknown });
		m_StoredConsoleStateHash = currentConsoleStateHash;
	}

	ConsoleInspectorTool::ImGuiRender(m_Variables, m_Functions, m_Unknowns);
}

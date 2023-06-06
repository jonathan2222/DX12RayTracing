#include "PreCompiled.h"
#include "ConsoleInspector.h"

#include "Core/Console.h"
#include "Render/ImGuiRenderer.h"
#include "Utils/Utils.h"

#include <vector>

using namespace RS;

void RSE::ConsoleInspector::Render()
{
	Console* pConsole = Console::Get();
	if (!pConsole) return;
	if (!m_Enabled) return;

	std::vector<Console::Variable> variables = pConsole->GetVariables(Console::s_VariableTypes);
	std::vector<Console::Variable> functions = pConsole->GetVariables({ Console::Type::Function });
	std::vector<Console::Variable> unknowns = pConsole->GetVariables({Console::Type::Unknown});
	if (variables.empty() == false)
	std::sort(variables.begin(), variables.end(),
		[](const Console::Variable& a, const Console::Variable& b) -> bool
		{
			return a.name < b.name;
		}
	);
	if (functions.empty() == false)
	std::sort(functions.begin(), functions.end(),
		[](const Console::Variable& a, const Console::Variable& b) -> bool
		{
			return a.name < b.name;
		}
	);
	if (unknowns.empty() == false)
	std::sort(unknowns.begin(), unknowns.end(),
		[](const Console::Variable& a, const Console::Variable& b) -> bool
		{
			return a.name < b.name;
		}
	);

	if (ImGui::Begin("Console Inspector"))
	{
		static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings;

		bool open = ImGui::CollapsingHeader("Variables", ImGuiTreeNodeFlags_DefaultOpen);
		if (open && ImGui::BeginTable("Table", 3, flags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 5)))
		{
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Data");
			ImGui::TableSetupColumn("Access");
			ImGui::TableHeadersRow();
			for (Console::Variable& var : variables)
				RenderVariable(var);

			ImGui::EndTable();
		}
		open = ImGui::CollapsingHeader("Functions", ImGuiTreeNodeFlags_DefaultOpen);
		if (open && ImGui::BeginTable("Table", 3, flags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 5)))
		{
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Data");
			ImGui::TableSetupColumn("Access");
			ImGui::TableHeadersRow();
			for (Console::Variable& var : functions)
				RenderVariable(var);

			ImGui::EndTable();
		}
		open = ImGui::CollapsingHeader("Unknowns", ImGuiTreeNodeFlags_DefaultOpen);
		if (open && ImGui::BeginTable("Table", 3, flags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 5)))
		{
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Data");
			ImGui::TableSetupColumn("Access");
			ImGui::TableHeadersRow();
			for (Console::Variable& var : unknowns)
				RenderVariable(var);

			ImGui::EndTable();
		}

		ImGui::End();
	}
}

void RSE::ConsoleInspector::RenderVariable(Console::Variable& var)
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

void RSE::ConsoleInspector::RenderFloatVariable(Console::Variable& var)
{
	ImGui::PushItemWidth(-1);
	float v = var.type == Console::Type::Float ? *(float*)var.pVar : (float)*(double*)var.pVar;
	uint32 numDecimals = Utils::GetNumDecimals(v);
	std::string format = Utils::Format("%.{}f", std::max(numDecimals, 1u));
	std::string floatLabel = Utils::Format("##Float-{}", var.name);
	if (ImGui::InputFloat(floatLabel.c_str(), &v, 0.0f, 0.0f, format.c_str(), ImGuiInputTextFlags_None))
	{
		if (var.type == Console::Type::Float) *(float*)var.pVar = v;
		else *(double*)var.pVar = (double)v;
	}
	ImGui::PopItemWidth();
}

void RSE::ConsoleInspector::RenderIntVariable(RS::Console::Variable& var)
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
	std::string floatLabel = Utils::Format("##Integer-{}", var.name);
	ImGui::InputScalar(floatLabel.c_str(), dataType, var.pVar, NULL, NULL, format, flags);
	ImGui::PopItemWidth();
}

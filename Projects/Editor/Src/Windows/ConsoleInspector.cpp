#include "PreCompiled.h"
#include "ConsoleInspector.h"

#include "Core/Console.h"
#include "Render/ImGuiRenderer.h"
#include "Utils/Utils.h"

#include "Tools/ConsoleInspectorTool.h"

#include <vector>

using namespace RS;

void RSE::ConsoleInspector::Render()
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

	if (ImGui::Begin(m_Name.c_str()))
	{
		ConsoleInspectorTool::ImGuiRender(m_Variables, m_Functions, m_Unknowns);
		ImGui::End();
	}
}

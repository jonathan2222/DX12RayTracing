#include "PreCompiled.h"
#include "DebugWindow.h"

#include "Render/ImGuiRenderer.h"

void RS::DebugWindow::SuperRender()
{
	if (m_Active)
	{
		if (ImGui::Begin(m_Name.c_str(), &m_Active))
		{
			Render();
			ImGui::End();
		}
	}
}

void RS::DebugWindow::UpdateShortcut()
{
	if (m_ShortcutKey != Key::UNKNOWN && m_ShortcutMods == Input::IGNORED)
		m_Shortcut = ToString(m_ShortcutKey);
	else if (m_ShortcutKey != Key::UNKNOWN)
		m_Shortcut = ToString(m_ShortcutKey) + std::string(" + ") + Input::ModFlagsToString(m_ShortcutMods);
}

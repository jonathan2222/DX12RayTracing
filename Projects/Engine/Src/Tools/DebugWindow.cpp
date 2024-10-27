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

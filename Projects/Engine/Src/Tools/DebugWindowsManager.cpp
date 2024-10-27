#include "PreCompiled.h"
#include "DebugWindowsManager.h"

#include "Render/ImGuiRenderer.h"
#include "GUI/LogNotifier.h"

#include "Core/Console.h"

#include "Tools/ConsoleInspectorTool.h"

void RS::DebugWindowsManager::Init()
{
    RegisterDebugWindow<DebugConsoleInspectorWindow>("Debug Console Inspector Window");

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

void RS::DebugWindowsManager::FixedTick()
{
	for (DebugWindow* pWindow : m_Windows)
		pWindow->SuperFixedTick();
}

void RS::DebugWindowsManager::Render()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    static bool dockspaceOpen = true;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DebugDockSpace", &dockspaceOpen, window_flags);
    ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding

    ImGui::PopStyleVar(2); // ImGuiStyleVar_WindowRounding, ImGuiStyleVar_WindowBorderSize

    // Submit dockspace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("DebugMyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspaceFlags);
    }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", NULL))
                RS_NOTIFY_ERROR("New is not implemented!");
            if (ImGui::MenuItem("Open", NULL))
                RS_NOTIFY_ERROR("Open is not implemented!");
            ImGui::Separator();

            if (ImGui::MenuItem("Close", NULL, false))
                RS::Display::Get()->Close();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows"))
        {
            RS::Console* pConsole = RS::Console::Get();
            bool selected = pConsole ? pConsole->IsEnabled() : false;
            if (ImGui::MenuItem("Console", (const char*)u8"\uE447", selected, pConsole))
            {
                if (selected) pConsole->Disable();
                else pConsole->Enable();
            }

            for (DebugWindow* pWindow : m_Windows)
            {
                bool oldState = pWindow->m_Active;
                if (ImGui::MenuItem(pWindow->m_Name.c_str(), "", &pWindow->m_Active, pWindow->GetEnableRequirements()))
                {
                    if (!oldState && pWindow->m_Active)
                        pWindow->SuperActivate();
                    else if (oldState && !pWindow->m_Active)
                        pWindow->SuperDeactivate();
                }
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    ImGui::End();

    for (DebugWindow* pWindow : m_Windows)
        pWindow->SuperRender();
}

void RS::DebugWindowsManager::Destory()
{
	for (DebugWindow* pWindow : m_Windows)
		delete pWindow;
}

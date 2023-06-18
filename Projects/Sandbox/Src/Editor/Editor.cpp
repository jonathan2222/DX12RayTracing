#include "PreCompiled.h"
#include "Editor.h"

#include "Core/Console.h"
#include "Core/Display.h"
#include "GUI/LogNotifier.h"

#include "Editor/Windows/ConsoleInspector.h"
#include "Editor/Windows/LifetimeTracker.h"
#include "Editor/Windows/Canvas.h"

RSE::Editor* RSE::Editor::Get()
{
    static std::unique_ptr<Editor> pEditor(new Editor());
    return pEditor.get();
}

void RSE::Editor::Init()
{
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Think this is for Multiple windows.

    RegisterEditorWindows();

    for (EditorWindow* pWindow : m_EditorWindows)
        pWindow->Init();
}

void RSE::Editor::Release()
{
    for (EditorWindow* pWindow : m_EditorWindows)
    {
        pWindow->Release();
        delete pWindow;
    }
    m_EditorWindows.clear();
}

void RSE::Editor::Update()
{
    for (EditorWindow* pWindow : m_EditorWindows)
        pWindow->SuperUpdate();
}

void RSE::Editor::FixedUpdate()
{
    for (EditorWindow* pWindow : m_EditorWindows)
        pWindow->SuperFixedUpdate();
}

void RSE::Editor::Render()
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

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (m_DockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    static bool dockspaceOpen = true;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
    ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding

    ImGui::PopStyleVar(2); // ImGuiStyleVar_WindowRounding, ImGuiStyleVar_WindowBorderSize

    // Submit dockspace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), m_DockspaceFlags);
    }

    RenderMenuBar();

    ImGui::End();

    for (EditorWindow* pWindow : m_EditorWindows)
        pWindow->SuperRender();
}

void RSE::Editor::Resize(uint32 width, uint32 height)
{
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

void RSE::Editor::RenderMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", NULL))
                RS_NOTIFY_ERROR("New is not implemented!");
            if (ImGui::MenuItem("Open", NULL))
                RS_NOTIFY_ERROR("Open is not implemented!");
            ImGui::Separator();

            if (ImGui::MenuItem("Flag: NoSplit", "", (m_DockspaceFlags & ImGuiDockNodeFlags_NoSplit) != 0)) { m_DockspaceFlags ^= ImGuiDockNodeFlags_NoSplit; }
            if (ImGui::MenuItem("Flag: NoResize", "", (m_DockspaceFlags & ImGuiDockNodeFlags_NoResize) != 0)) { m_DockspaceFlags ^= ImGuiDockNodeFlags_NoResize; }
            if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (m_DockspaceFlags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0)) { m_DockspaceFlags ^= ImGuiDockNodeFlags_NoDockingInCentralNode; }
            if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (m_DockspaceFlags & ImGuiDockNodeFlags_AutoHideTabBar) != 0)) { m_DockspaceFlags ^= ImGuiDockNodeFlags_AutoHideTabBar; }
            if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (m_DockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) != 0)) { m_DockspaceFlags ^= ImGuiDockNodeFlags_PassthruCentralNode; }
            ImGui::Separator();

            if (ImGui::MenuItem("Close", NULL, false))
                RS::Display::Get()->Close();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows"))
        {
            RS::Console* pConsole = RS::Console::Get();
            bool selected = pConsole ? pConsole->IsEnabled() : false;
            if (ImGui::MenuItem("Console", u8"\uE447", selected, pConsole))
            {
                if (selected) pConsole->Disable();
                else pConsole->Enable();
            }

            for (EditorWindow* pWindow : m_EditorWindows)
                ImGui::MenuItem(pWindow->GetName().c_str(), "", &pWindow->GetEnable(), pWindow->GetEnableRequirements());

            ImGui::EndMenu();
        }

        ImGui::HelpMarker(
            "When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n"
            "- Drag from window title bar or their tab to dock/undock." "\n"
            "- Drag from window menu button (upper-left button) to undock an entire node (all windows)." "\n"
            "- Hold SHIFT to disable docking (if io.ConfigDockingWithShift == false, default)" "\n"
            "- Hold SHIFT to enable docking (if io.ConfigDockingWithShift == true)" "\n"
            "This demo app has nothing to do with enabling docking!" "\n\n"
            "This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window." "\n\n"
            "Read comments in ShowExampleAppDockSpace() for more details.");

        ImGui::EndMenuBar();
    }
}

void RSE::Editor::RegisterEditorWindows()
{
    RegisterEditorWindow<ConsoleInspector>("Console Inspector");
    RegisterEditorWindow<LifetimeTracker>("Lifetime Tracker");
    RegisterEditorWindow<Canvas>("Canvas", true);
}

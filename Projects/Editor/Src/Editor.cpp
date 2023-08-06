#include "PreCompiled.h"
#include "Editor.h"

#include "Core/Console.h"
#include "Core/Display.h"
#include "GUI/LogNotifier.h"

#include "Windows/ConsoleInspector.h"
#include "Windows/LifetimeTracker.h"
#include "Windows/Canvas.h"

RSE::Editor* RSE::Editor::Get()
{
    static std::unique_ptr<Editor> pEditor(new Editor());
    return pEditor.get();
}

void RSE::Editor::Init()
{
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // For Multiple windows. TODO: Need to implemented this in the ImGuiRenderer first!

    RS::Console* pConsole = RS::Console::Get();
    pConsole->AddFunction("Editor.CloseAllWindows", [this](RS::Console::FuncArgs args)
        {
            if (!RS::Console::ValidateFuncArgs(args, { {"-a"} }, 0))
                return;

            bool closeCanvasWindow = args.Get("-a").has_value();
            for (EditorWindow* pWindow : m_EditorWindows)
                if (closeCanvasWindow || dynamic_cast<Canvas*>(pWindow) == nullptr)
                    pWindow->GetEnable() = false;
            m_ShowImGuiDemoWindow = false;
            m_ShowToastDemoWindow = false;
        },
        RS::Console::Flag::NONE, "Closes all windows except the Canvas window.\nUse '-a' to also close the canvas window."
    );

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

    // Debug stuff
    if (m_ShowImGuiDemoWindow)
        ImGui::ShowDemoWindow(&m_ShowImGuiDemoWindow);
    if (m_ShowToastDemoWindow)
        RenderToastDemo();
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
            
            if (ImGui::BeginMenu("Other"))
            {
                ImGui::MenuItem("ImGui Demo", "", &m_ShowImGuiDemoWindow);
                ImGui::MenuItem("Toast Demo", "", &m_ShowToastDemoWindow);
                ImGui::EndMenu();
            }

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

void RSE::Editor::RenderToastDemo()
{
    static uint32_t position = ImGui::GetDefaultNotificationPosition();

    ImGui::Begin("Notification Test", &m_ShowToastDemoWindow);

    if (ImGui::Button("Warning"))
        ImGui::InsertNotification({ ImGuiToastType_Warning, 3000, "Hello World! This is a warning! {}", 0x1337 });
    if (ImGui::Button("Success"))
        ImGui::InsertNotification({ ImGuiToastType_Success, 3000, "Hello World! This is a success! {}", "We can also format here:)" });
    if (ImGui::Button("Error"))
        ImGui::InsertNotification({ ImGuiToastType_Error, 3000, "Hello World! This is an error! {:#10X}", 0xDEADBEEF });
    if (ImGui::Button("Info"))
        ImGui::InsertNotification({ ImGuiToastType_Info, 3000, "Hello World! This is an info!" });
    if (ImGui::Button("Info Long"))
        ImGui::InsertNotification({ ImGuiToastType_Info, 3000, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation" });

    if (ImGui::Button("Critical"))
        ImGui::InsertNotification({ ImGuiToastType_Critical, "Hello World! This is an info!" });
    if (ImGui::Button("Debug"))
        ImGui::InsertNotification({ ImGuiToastType_Debug, "Hello World! This is an info!" });

    if (ImGui::Button("Custom title"))
    {
        // Now using a custom title...
        ImGuiToast toast(ImGuiToastType_Success, 3000); // <-- content can also be passed here as above
        toast.set_title("This is a {} title", "wonderful");
        toast.set_content("Lorem ipsum dolor sit amet");
        ImGui::InsertNotification(toast);
    }

    if (ImGui::Button("No dismiss"))
    {
        ImGui::InsertNotification({ ImGuiToastType_Error, IMGUI_NOTIFY_NO_DISMISS, "Test 0x%X", 0xDEADBEEF }).bind([]()
            {
                ImGui::InsertNotification({ ImGuiToastType_Info, "Signaled" });
            }
        ).dismissOnSignal();
    }

    if (ImGui::BeginPopupContextWindow())
    {
        if (ImGui::MenuItem("Top-left", NULL, position == ImGuiToastPos_TopLeft)) { position = ImGuiToastPos_TopLeft; ImGui::SetNotificationPosition(position); }
        if (ImGui::MenuItem("Top-Center", NULL, position == ImGuiToastPos_TopCenter)) { position = ImGuiToastPos_TopCenter; ImGui::SetNotificationPosition(position); }
        if (ImGui::MenuItem("Top-right", NULL, position == ImGuiToastPos_TopRight)) { position = ImGuiToastPos_TopRight; ImGui::SetNotificationPosition(position); }
        if (ImGui::MenuItem("Bottom-left", NULL, position == ImGuiToastPos_BottomLeft)) { position = ImGuiToastPos_BottomLeft; ImGui::SetNotificationPosition(position); }
        if (ImGui::MenuItem("Bottom-Center", NULL, position == ImGuiToastPos_BottomCenter)) { position = ImGuiToastPos_BottomCenter; ImGui::SetNotificationPosition(position); }
        if (ImGui::MenuItem("Bottom-right", NULL, position == ImGuiToastPos_BottomRight)) { position = ImGuiToastPos_BottomRight; ImGui::SetNotificationPosition(position); }
        ImGui::EndPopup();
    }

    ImGui::End();
}

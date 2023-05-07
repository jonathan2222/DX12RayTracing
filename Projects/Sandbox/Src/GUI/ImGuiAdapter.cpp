#include "PreCompiled.h"
#include "ImGuiAdapter.h"

#include <imgui.h>
#include <backends/imgui_impl_dx12.h>

#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>   // for glfwGetWin32Window
#endif

using namespace RS;

#define GLFW_HAS_WINDOW_TOPMOST       (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3200) // 3.2+ GLFW_FLOATING
#define GLFW_HAS_WINDOW_HOVERED       (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ GLFW_HOVERED
#define GLFW_HAS_WINDOW_ALPHA         (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ glfwSetWindowOpacity
#define GLFW_HAS_PER_MONITOR_DPI      (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ glfwGetMonitorContentScale
#define GLFW_HAS_VULKAN               (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3200) // 3.2+ glfwCreateWindowSurface
#ifdef GLFW_RESIZE_NESW_CURSOR  // let's be nice to people who pulled GLFW between 2019-04-16 (3.4 define) and 2019-11-29 (cursors defines) // FIXME: Remove when GLFW 3.4 is released?
#define GLFW_HAS_NEW_CURSORS          (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3400) // 3.4+ GLFW_RESIZE_ALL_CURSOR, GLFW_RESIZE_NESW_CURSOR, GLFW_RESIZE_NWSE_CURSOR, GLFW_NOT_ALLOWED_CURSOR
#else
#define GLFW_HAS_NEW_CURSORS          (0)
#endif

bool RS::ImGuiAdapter::Init(GLFWwindow* window, bool install_callbacks, ClientAPI clientAPI)
{
    s_Window = window;
    s_Time = 0.0;

    // Setup backend capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendPlatformName = "imgui_impl_glfw";

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.SetClipboardTextFn = ImGuiAdapter::SetClipboardText;
    io.GetClipboardTextFn = ImGuiAdapter::GetClipboardText;
    io.ClipboardUserData = s_Window;

    /*   ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    #if defined(_WIN32)
        io.ImeWindowHandle = (void*)glfwGetWin32Window(s_Window);
    #endif
    */

    // Create mouse cursors
    // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
    // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
    // Missing cursors will return NULL and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
    GLFWerrorfun prev_error_callback = glfwSetErrorCallback(NULL);
    s_MouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if GLFW_HAS_NEW_CURSORS
    s_MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
#else
    s_MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
#endif
    glfwSetErrorCallback(prev_error_callback);

    // Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
    s_PrevUserCallbackMousebutton = NULL;
    s_PrevUserCallbackScroll = NULL;
    s_PrevUserCallbackKey = NULL;
    s_PrevUserCallbackChar = NULL;
    if (install_callbacks)
    {
        s_InstalledCallbacks = true;
        s_PrevUserCallbackMousebutton = glfwSetMouseButtonCallback(window, ImGuiAdapter::MouseButtonCallback);
        s_PrevUserCallbackScroll = glfwSetScrollCallback(window, ImGuiAdapter::ScrollCallback);
        s_PrevUserCallbackKey = glfwSetKeyCallback(window, ImGuiAdapter::KeyCallback);
        s_PrevUserCallbackChar = glfwSetCharCallback(window, ImGuiAdapter::CharCallback);
    }

    s_ClientAPI = clientAPI;
    return true;
}

void RS::ImGuiAdapter::Release()
{
    if (s_InstalledCallbacks)
    {
        glfwSetMouseButtonCallback(s_Window, s_PrevUserCallbackMousebutton);
        glfwSetScrollCallback(s_Window, s_PrevUserCallbackScroll);
        glfwSetKeyCallback(s_Window, s_PrevUserCallbackKey);
        glfwSetCharCallback(s_Window, s_PrevUserCallbackChar);
        s_InstalledCallbacks = false;
    }

    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
    {
        glfwDestroyCursor(s_MouseCursors[cursor_n]);
        s_MouseCursors[cursor_n] = NULL;
    }
    s_ClientAPI = ClientAPI::UNKNOWN;
}

void RS::ImGuiAdapter::NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(s_Window, &w, &h);
    glfwGetFramebufferSize(s_Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);

    // Setup time step
    double current_time = glfwGetTime();
    io.DeltaTime = s_Time > 0.0 ? (float)(current_time - s_Time) : (float)(1.0f / 60.0f);
    s_Time = current_time;

    if (s_IsInputActive)
    {
        UpdateMousePosAndButtons();
        UpdateMouseCursor();

        // Update game controllers (if enabled and available)
        UpdateGamepads();
    }
}

void RS::ImGuiAdapter::DisableInput()
{
    s_IsInputActive = false;
}

void RS::ImGuiAdapter::EnableInput()
{
    s_IsInputActive = true;
}

void RS::ImGuiAdapter::UpdateMousePosAndButtons()
{
    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
    {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = s_MouseJustPressed[i] || glfwGetMouseButton(s_Window, i) != 0;
        s_MouseJustPressed[i] = false;
    }

    // Update mouse position
    const ImVec2 mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
#ifdef __EMSCRIPTEN__
    const bool focused = true; // Emscripten
#else
    const bool focused = glfwGetWindowAttrib(s_Window, GLFW_FOCUSED) != 0;
#endif
    if (focused)
    {
        if (io.WantSetMousePos)
        {
            glfwSetCursorPos(s_Window, (double)mouse_pos_backup.x, (double)mouse_pos_backup.y);
        }
        else
        {
            double mouse_x, mouse_y;
            glfwGetCursorPos(s_Window, &mouse_x, &mouse_y);
            io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
        }
    }
}

void RS::ImGuiAdapter::UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(s_Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        glfwSetInputMode(s_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else
    {
        // Show OS mouse cursor
        // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
        glfwSetCursor(s_Window, s_MouseCursors[imgui_cursor] ? s_MouseCursors[imgui_cursor] : s_MouseCursors[ImGuiMouseCursor_Arrow]);
        glfwSetInputMode(s_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void RS::ImGuiAdapter::UpdateGamepads()
{
    ImGuiIO& io = ImGui::GetIO();
    memset(io.NavInputs, 0, sizeof(io.NavInputs));
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;

    // Update gamepad inputs
#define MAP_BUTTON(NAV_NO, BUTTON_NO)       { if (buttons_count > BUTTON_NO && buttons[BUTTON_NO] == GLFW_PRESS) io.NavInputs[NAV_NO] = 1.0f; }
#define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float v = (axes_count > AXIS_NO) ? axes[AXIS_NO] : V0; v = (v - V0) / (V1 - V0); if (v > 1.0f) v = 1.0f; if (io.NavInputs[NAV_NO] < v) io.NavInputs[NAV_NO] = v; }
    int axes_count = 0, buttons_count = 0;
    const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
    const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttons_count);
    MAP_BUTTON(ImGuiNavInput_Activate, 0);     // Cross / A
    MAP_BUTTON(ImGuiNavInput_Cancel, 1);     // Circle / B
    MAP_BUTTON(ImGuiNavInput_Menu, 2);     // Square / X
    MAP_BUTTON(ImGuiNavInput_Input, 3);     // Triangle / Y
    MAP_BUTTON(ImGuiNavInput_DpadLeft, 13);    // D-Pad Left
    MAP_BUTTON(ImGuiNavInput_DpadRight, 11);    // D-Pad Right
    MAP_BUTTON(ImGuiNavInput_DpadUp, 10);    // D-Pad Up
    MAP_BUTTON(ImGuiNavInput_DpadDown, 12);    // D-Pad Down
    MAP_BUTTON(ImGuiNavInput_FocusPrev, 4);     // L1 / LB
    MAP_BUTTON(ImGuiNavInput_FocusNext, 5);     // R1 / RB
    MAP_BUTTON(ImGuiNavInput_TweakSlow, 4);     // L1 / LB
    MAP_BUTTON(ImGuiNavInput_TweakFast, 5);     // R1 / RB
    MAP_ANALOG(ImGuiNavInput_LStickLeft, 0, -0.3f, -0.9f);
    MAP_ANALOG(ImGuiNavInput_LStickRight, 0, +0.3f, +0.9f);
    MAP_ANALOG(ImGuiNavInput_LStickUp, 1, +0.3f, +0.9f);
    MAP_ANALOG(ImGuiNavInput_LStickDown, 1, -0.3f, -0.9f);
#undef MAP_BUTTON
#undef MAP_ANALOG
    if (axes_count > 0 && buttons_count > 0)
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    else
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
}

const char* RS::ImGuiAdapter::GetClipboardText(void* user_data)
{
    return glfwGetClipboardString((GLFWwindow*)user_data);
}

void RS::ImGuiAdapter::SetClipboardText(void* user_data, const char* text)
{
    glfwSetClipboardString((GLFWwindow*)user_data, text);
}

void RS::ImGuiAdapter::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (s_PrevUserCallbackMousebutton != NULL)
        s_PrevUserCallbackMousebutton(window, button, action, mods);

    if (s_IsInputActive)
    {
        if (action == GLFW_PRESS && button >= 0 && button < IM_ARRAYSIZE(s_MouseJustPressed))
            s_MouseJustPressed[button] = true;
    }
}

void RS::ImGuiAdapter::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (s_PrevUserCallbackScroll != NULL)
        s_PrevUserCallbackScroll(window, xoffset, yoffset);

    if (s_IsInputActive)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseWheelH += (float)xoffset;
        io.MouseWheel += (float)yoffset;
    }
}

void RS::ImGuiAdapter::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (s_PrevUserCallbackKey != NULL)
        s_PrevUserCallbackKey(window, key, scancode, action, mods);

    if (s_IsInputActive)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (action == GLFW_PRESS)
            io.KeysDown[key] = true;
        if (action == GLFW_RELEASE)
            io.KeysDown[key] = false;

        // Modifiers are not reliable across systems
        io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
        io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
        io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
#ifdef _WIN32
        io.KeySuper = false;
#else
        io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
#endif
    }
}

void RS::ImGuiAdapter::CharCallback(GLFWwindow* window, unsigned int c)
{
    if (s_PrevUserCallbackChar != NULL)
        s_PrevUserCallbackChar(window, c);

    if (s_IsInputActive)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.AddInputCharacter(c);
    }
}

#include "PreCompiled.h"
#include "Input.h"

#include "Core/Display.h"

#include "GUI/ImGuiAdapter.h"
#include "Render/ImGuiRenderer.h"

using namespace RS;

std::shared_ptr<Input> Input::Get()
{
    static std::shared_ptr<Input> s_Input = std::make_shared<Input>();
    return s_Input;
}

void Input::Init()
{
    GLFWwindow* wnd = static_cast<GLFWwindow*>(Display::Get()->GetGLFWWindow());
    glfwSetKeyCallback(wnd, Input::KeyCallback);
    glfwSetCursorPosCallback(wnd, Input::CursorPositionCallback);
    glfwSetMouseButtonCallback(wnd, Input::MouseButtonCallback);
    glfwSetScrollCallback(wnd, Input::MouseScrollCallback);
}

void Input::PreUpdate()
{
    s_MouseDelta = s_MousePos - s_MousePosPre;
}

void Input::PostUpdate(float dt)
{
    for (auto& key : s_KeyMap)
        UpdateKeyInfo(key.second, dt);
    for (auto& mb : s_MBMap)
        UpdateKeyInfo(mb.second, dt);

    s_MousePosPre = s_MousePos;
    s_ScrollDelta = glm::vec2(0.f);
}

bool Input::IsKeyPressed(const Key& key, ModFlags mods) const
{
    KeyInfo& keyInfo = GetKeyInfo(key);
    return InternalIsKeyPressed(keyInfo, mods);
}

bool Input::IsKeyClicked(const Key& key, ActiveState activeState, ModFlags mods) const
{
    KeyInfo& keyInfo = GetKeyInfo(key);
    return InternalIsKeyClicked(keyInfo, activeState, mods);
}

bool Input::WasKeyClickedOrHeld(const Key& key, std::function<void(void)> onClicked, std::function<void(void)> onHeld, float timePeriod, ModFlags mods)
{
    KeyInfo& keyInfo = GetKeyInfo(key);
    return InternalWasKeyClickedOrHeld(keyInfo, onClicked, onHeld, timePeriod, mods);
}

KeyState Input::GetKeyState(const Key& key) const
{
    KeyInfo& keyInfo = GetKeyInfo(key);
    return keyInfo.State;
}

float Input::GetKeyTime(const Key& key) const
{
    KeyInfo& keyInfo = GetKeyInfo(key);
    return keyInfo.TimeHeld;
}

Input::ModFlags Input::GetKeyMods(const Key& key) const
{
    KeyInfo& keyInfo = GetKeyInfo(key);
    return keyInfo.Mods;
}

glm::vec2 Input::GetCursorDelta()
{
    return s_MouseDelta;
}

glm::vec2 Input::GetScrollDelta()
{
    return s_ScrollDelta;
}

glm::vec2 Input::GetMousePos() const
{
    return s_MousePos;
}

bool Input::IsMBPressed(const MB& button, ModFlags mods) const
{
    KeyInfo& keyInfo = GetKeyInfo(button);
    return InternalIsKeyPressed(keyInfo, mods);
}

bool Input::IsMBClicked(const MB& button, ActiveState activeState, ModFlags mods) const
{
    KeyInfo& keyInfo = GetKeyInfo(button);
    return InternalIsKeyClicked(keyInfo, activeState, mods);
}

bool Input::WasMBClickedOrHeld(const MB& button, std::function<void(void)> onClicked, std::function<void(void)> onHeld, float timePeriod, ModFlags mods)
{
    KeyInfo& keyInfo = GetKeyInfo(button);
    return InternalWasKeyClickedOrHeld(keyInfo, onClicked, onHeld, timePeriod, mods);
}

KeyState Input::GetMBState(const MB& button) const
{
    KeyInfo& keyInfo = GetKeyInfo(button);
    return keyInfo.State;
}

float Input::GetMBTime(const MB& button) const
{
    KeyInfo& keyInfo = GetKeyInfo(button);
    return keyInfo.TimeHeld;
}

Input::ModFlags Input::GetMBMods(const MB& button) const
{
    KeyInfo& keyInfo = GetKeyInfo(button);
    return keyInfo.Mods;
}

void Input::CenterMouse() const
{
    GLFWwindow* wnd = static_cast<GLFWwindow*>(Display::Get()->GetGLFWWindow());
    s_MouseDelta = s_MousePos - s_MousePosPre;
    s_MousePos.x = (float)Display::Get()->GetWidth() * 0.5f;
    s_MousePos.y = (float)Display::Get()->GetHeight() * 0.5f;
    glfwSetCursorPos(wnd, (double)s_MousePos.x, (double)s_MousePos.y);
}

void Input::LockMouse() const
{
    ImGuiAdapter::DisableInput();

    GLFWwindow* wnd = static_cast<GLFWwindow*>(Display::Get()->GetGLFWWindow());
    glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    s_MousePosPre = s_MousePos;
    s_MouseDelta.x = 0.0f;
    s_MouseDelta.y = 0.0f;
}

void Input::UnlockMouse() const
{
    ImGuiAdapter::EnableInput();

    GLFWwindow* wnd = static_cast<GLFWwindow*>(Display::Get()->GetGLFWWindow());
    glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    s_MousePosPre = s_MousePos;
    s_MouseDelta.x = 0.0f;
    s_MouseDelta.y = 0.0f;
}

std::string Input::KeyStateToStr(KeyState state)
{
    std::string str;
    switch (state)
    {
    case KeyState::RELEASED:
        str = "RELEASED";
        break;
    case KeyState::FIRST_RELEASED:
        str = "FIRST_RELEASED";
        break;
    case KeyState::PRESSED:
        str = "PRESSED";
        break;
    case KeyState::FIRST_PRESSED:
        str = "FIRST_PRESSED";
        break;
    default:
        str = "UNKNOWN_STATE";
        break;
    }
    return str;
}

bool Input::InternalIsKeyPressed(const KeyInfo& keyInfo, ModFlags mods) const
{
    bool pressed = keyInfo.State == KeyState::FIRST_PRESSED || keyInfo.State == KeyState::PRESSED;
    if (mods != ModFlag::IGNORED)
        pressed = pressed && ((keyInfo.Mods & mods) != 0 || keyInfo.Mods == mods);
    return pressed;
}

bool Input::InternalIsKeyClicked(const KeyInfo& keyInfo, ActiveState activeState, ModFlags mods) const
{
    bool pressed = keyInfo.State == (activeState == ActiveState::FALLING_EDGE ? KeyState::FIRST_RELEASED : KeyState::FIRST_PRESSED);
    if (mods != ModFlag::IGNORED)
        pressed = pressed && ((keyInfo.Mods & mods) != 0 || keyInfo.Mods == mods);
    return pressed;
}

bool Input::InternalWasKeyClickedOrHeld(const KeyInfo& keyInfo, std::function<void(void)> onClicked, std::function<void(void)> onHeld, float timePeriod, ModFlags mods)
{
    float timeHeld = keyInfo.TimeHeld;
    if (InternalIsKeyClicked(keyInfo, ActiveState::FALLING_EDGE, mods))
    {
        // Count as a click if the time it was held was less than timePeriod (in seconds).
        if (timeHeld <= timePeriod)
            onClicked();
        else
            onHeld();
        return true;
    }
    return false;
}

Input::KeyInfo& Input::GetKeyInfo(const Key& key) const
{
    return s_KeyMap[key];
}

Input::KeyInfo& Input::GetKeyInfo(const MB& button) const
{
    return s_MBMap[button];
}

void Input::UpdateKeyInfo(KeyInfo& keyInfo, float dt)
{
    if (keyInfo.State == KeyState::FIRST_PRESSED)
        keyInfo.State = KeyState::PRESSED;

    if (keyInfo.State == KeyState::FIRST_RELEASED)
        keyInfo.State = KeyState::RELEASED;

    if (keyInfo.State == KeyState::PRESSED)
        keyInfo.TimeHeld += dt;
    else
        keyInfo.TimeHeld = 0.f;
}

void Input::KeyCallback(GLFWwindow* wnd, int key, int scancode, int action, int mods)
{
    RS_UNREFERENCED_VARIABLE(wnd);
    RS_UNREFERENCED_VARIABLE(scancode);
    RS_UNREFERENCED_VARIABLE(mods);
    if (!ImGuiRenderer::Get()->WantKeyInput())
    {
        Key ymKey = (Key)key;
        if (action == GLFW_PRESS)
            s_KeyMap[ymKey].State = KeyState::FIRST_PRESSED;
        if (action == GLFW_RELEASE)
            s_KeyMap[ymKey].State = KeyState::FIRST_RELEASED;
        s_KeyMap[ymKey].Mods = mods;
    }
}

void Input::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    RS_UNREFERENCED_VARIABLE(window);
    {
        s_MousePos.x = (float)xpos;
        s_MousePos.y = (float)ypos;
    }
}

void Input::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    RS_UNREFERENCED_VARIABLE(window);
    RS_UNREFERENCED_VARIABLE(mods);
    if (!ImGuiRenderer::Get()->WantKeyInput())
    {
        MB ymButton = (MB)button;
        if (action == GLFW_PRESS)
            s_MBMap[ymButton].State = KeyState::FIRST_PRESSED;
        if (action == GLFW_RELEASE)
            s_MBMap[ymButton].State = KeyState::FIRST_RELEASED;
        s_MBMap[ymButton].Mods = mods;
    }
}

void Input::MouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    RS_UNREFERENCED_VARIABLE(window);
    if (!ImGuiRenderer::Get()->WantKeyInput())
    {
        s_ScrollDelta.x = (float)xOffset;
        s_ScrollDelta.y = (float)yOffset;
    }
}

#include "PreCompiled.h"
#include "EngineLoop.h"

#include "Core/Display.h"
#include "Core/Input.h"

#include "DX12/Dx12Core.h"

using namespace RS;

std::shared_ptr<EngineLoop> EngineLoop::Get()
{
    static std::shared_ptr<EngineLoop> s_EngineLoop = std::make_shared<EngineLoop>();
    return s_EngineLoop;
}

void EngineLoop::Init()
{
    std::shared_ptr<RS::Display> pDisplay = RS::Display::Get();
    Dx12Core::Get()->Init(pDisplay->GetWidth(), pDisplay->GetHeight(), pDisplay->GetHWND());
}

void EngineLoop::Release()
{
    Dx12Core::Get()->Release();
}

void EngineLoop::Run()
{
    std::shared_ptr<RS::Display> pDisplay = RS::Display::Get();

    m_FrameTimer.Init(&m_FrameStats, 0.25f);
    while (!pDisplay->ShouldClose())
    {
        m_FrameTimer.Begin();

        pDisplay->PollEvents();
        RS::Input::Get()->PreUpdate();

        // Quit by pressing ESCAPE
        {
            if (RS::Input::Get()->IsKeyPressed(RS::Key::ESCAPE))
                pDisplay->Close();
        }

        // Toggle fullscreen by pressing F11
        {
            if (RS::Input::Get()->IsKeyClicked(RS::Key::F11))
                pDisplay->ToggleFullscreen();
        }

        m_FrameTimer.FixedTick([&]() { FixedTick(); });
        Tick(m_FrameStats);

        RS::Input::Get()->PostUpdate(m_FrameStats.frame.currentDT);

        m_FrameTimer.End();
    }
}

void EngineLoop::FixedTick()
{

}

void EngineLoop::Tick(const RS::FrameStats& frameStats)
{
    Dx12Core::Get()->Render();
}

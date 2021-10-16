#include <iostream>

#include "Core/Display.h"
#include "Core/Input.h"
#include "Core/FrameTimer.h"

void Run();
void FixedTick();
void Tick(const RS::FrameStats& frameStats);

int main()
{
    RS::Logger::Init();

    RS::DisplayDescription displayDesc = {};
    displayDesc.Title       = "Sandbox DX12";
    displayDesc.Width       = 1920;
    displayDesc.Height      = 1080;
    displayDesc.Fullscreen  = false;
    displayDesc.VSync       = true;
    RS::Display::Get()->Init(displayDesc);
    RS::Input::Get()->Init();

    Run();

    RS::Display::Get()->Release();
    return 0;
}

void Run()
{
    std::shared_ptr<RS::Display> pDisplay = RS::Display::Get();

    RS::FrameStats frameStats = {};
    RS::FrameTimer frameTimer;
    frameTimer.Init(&frameStats, 0.25f);
    while (!pDisplay->ShouldClose())
    {
        frameTimer.Begin();

        pDisplay->PollEvents();
        RS::Input::Get()->PreUpdate();

        // Quit by pressing ESCAPE
        {
            if (RS::Input::Get()->IsKeyPressed(RS::Key::ESCAPE))
                pDisplay->Close();
        }

        // Toggle fullscreen by pressing F11
        {
            static bool s_F11Active = true;
            RS::KeyState f11State = RS::Input::Get()->GetKeyState(RS::Key::F11);
            if (f11State == RS::KeyState::PRESSED && s_F11Active)
            {
                pDisplay->ToggleFullscreen();
                s_F11Active = false;
            }
            else if (f11State == RS::KeyState::RELEASED)
                s_F11Active = true;
        }

        frameTimer.FixedTick([&]() { FixedTick(); });
        Tick(frameStats);

        RS::Input::Get()->PostUpdate(frameStats.frame.currentDT);

        frameTimer.End();
    }
}

void FixedTick()
{

}

void Tick(const RS::FrameStats& frameStats)
{

}
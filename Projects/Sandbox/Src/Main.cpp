#include <iostream>

#include "Core/Config.h"
#include "Core/Display.h"
#include "Core/Input.h"
#include "Core/FrameTimer.h"

void Run();
void FixedTick();
void Tick(const RS::FrameStats& frameStats);

int main()
{
    RS::Logger::Init();
    RS::Config::Get()->Init(RS_CONFIG_FILE_PATH);

    RS::DisplayDescription displayDesc = {};
    displayDesc.Title       = RS::Config::Get()->Fetch<std::string>("Display/Title", "Sandbox DX12");
    displayDesc.Width       = RS::Config::Get()->Fetch<uint32>("Display/DefaultWidth", 1920);
    displayDesc.Height      = RS::Config::Get()->Fetch<uint32>("Display/DefaultHeight", 1080);
    displayDesc.Fullscreen  = RS::Config::Get()->Fetch<bool>("Display/Fullscreen", false);
    displayDesc.VSync       = RS::Config::Get()->Fetch<bool>("Display/VSync", true);
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
            if (RS::Input::Get()->IsKeyClicked(RS::Key::F11))
                pDisplay->ToggleFullscreen();
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
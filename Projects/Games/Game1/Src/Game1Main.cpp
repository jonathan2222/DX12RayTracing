#include <iostream>

#include <RSEngine.h>
#include "Game1App.h"

int main(int argc, char* argv[])
{
    RS::Engine::SetInternalDataFilePath("../../../Assets/");
    RS::Engine::SetDataFilePath("./Assets/");
    RS::Engine::SetDebugFilePath("./Debug/");
    RS::Engine::SetTempFilePath("./Temp/");

    RS::Logger::Init();
    RS::LaunchArguments::Init(argc, argv);
    RS::Config::Get()->Init("Config/Game1.cfg");

    RS::DisplayDescription displayDesc = {};
    displayDesc.Title                   = RS::Config::Map.Fetch("Display/InitialState/Title", "Game1");
    displayDesc.Width                   = RS::Config::Map.Fetch("Display/InitialState/DefaultWidth", 1920u);
    displayDesc.Height                  = RS::Config::Map.Fetch("Display/InitialState/DefaultHeight", 1080u);
    displayDesc.Fullscreen              = RS::Config::Map.Fetch("Display/InitialState/Fullscreen", false);
    displayDesc.UseWindowedFullscreen   = RS::Config::Map.Fetch("Display/Settings/WindowedFullscreen", false);
    displayDesc.VSync                   = RS::Config::Map.Fetch("Display/Settings/VSync", true);
    RS::Display::Get()->Init(displayDesc);
    RS::Input::Get()->Init();

    auto pEngineLoop = RS::EngineLoop::Get();
    pEngineLoop->Init();
    {
        Game1App app; // Need to be here such that it calls descructor before we release the engine loop.
        pEngineLoop->additionalFixedTickFunction = [&]() { app.FixedTick(); };
        pEngineLoop->additionalTickFunction = [&](const RS::FrameStats& frameStats) { app.Tick(frameStats); };
        RS::Display::Get()->AddOnSizeChangeCallback("EngineLoop SizeChangeCallback", dynamic_cast<RS::IDisplaySizeChange*>(pEngineLoop.get()));
        pEngineLoop->Run();
    }
    pEngineLoop->Release();

    displayDesc = RS::Display::Get()->GetDescription();
    RS::Config::Map["Display/InitialState/Title"] = displayDesc.Title;
    RS::Config::Map["Display/InitialState/DefaultWidth"] = displayDesc.Width;
    RS::Config::Map["Display/InitialState/DefaultHeight"] = displayDesc.Height;
    RS::Config::Map["Display/InitialState/Fullscreen"] = displayDesc.Fullscreen;
    RS::Config::Map["Display/Settings/WindowedFullscreen"] = displayDesc.UseWindowedFullscreen;
    RS::Config::Map["Display/Settings/VSync"] = displayDesc.VSync;

    RS::Display::Get()->Release();
    RS::Config::Get()->Destroy();
    RS::LaunchArguments::Release();
    return 0;
}

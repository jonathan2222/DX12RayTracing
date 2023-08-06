#include <iostream>

#include "RSEngine.h"

void FixedTick();
void Tick();

int main(int argc, char* argv[])
{
    RS::Logger::Init();
    RS::LaunchArguments::Init(argc, argv);
    RS::Config::Get()->Init(RS_CONFIG_FILE_PATH);

    RS::DisplayDescription displayDesc = {};
    displayDesc.Title                   = RS::Config::Get()->Fetch<std::string>("Display/InitialState/Title", "Sandbox DX12");
    displayDesc.Width                   = RS::Config::Get()->Fetch<uint32>("Display/InitialState/DefaultWidth", 1920);
    displayDesc.Height                  = RS::Config::Get()->Fetch<uint32>("Display/InitialState/DefaultHeight", 1080);
    displayDesc.Fullscreen              = RS::Config::Get()->Fetch<bool>("Display/InitialState/Fullscreen", false);
    displayDesc.UseWindowedFullscreen   = RS::Config::Get()->Fetch<bool>("Display/Settings/WindowedFullscreen", false);
    displayDesc.VSync                   = RS::Config::Get()->Fetch<bool>("Display/Settings/VSync", true);
    RS::Display::Get()->Init(displayDesc);
    RS::Input::Get()->Init();

    auto pEngineLook = RS::EngineLoop::Get();
    pEngineLook->Init();
    pEngineLook->additionalFixedTickFunction = FixedTick;
    pEngineLook->additionalTickFunction = Tick;
    RS::Display::Get()->SetOnSizeChangeCallback(dynamic_cast<RS::IDisplaySizeChange*>(pEngineLook.get()));
    pEngineLook->Run();
    pEngineLook->Release();

    RS::Display::Get()->Release();
    RS::LaunchArguments::Release();
    return 0;
}

void FixedTick()
{

}

void Tick()
{

}
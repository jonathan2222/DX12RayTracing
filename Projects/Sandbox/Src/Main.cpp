#include <iostream>

#include "Core/Config.h"
#include "Core/Display.h"
#include "Core/Input.h"
#include "Core/EngineLoop.h"

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

    RS::EngineLoop::Get()->Init();
    RS::EngineLoop::Get()->Run();

    RS::EngineLoop::Get()->Release();
    RS::Display::Get()->Release();
    return 0;
}
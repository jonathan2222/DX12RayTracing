#include <iostream>

#include "RSEngine.h"
#include "Editor.h"

#include "Core/VMap.h"

int main(int argc, char* argv[])
{
    RS::Logger::Init();
    RS::LaunchArguments::Init(argc, argv);
    RS::Config::Get()->Init(RS_CONFIG_FILE_PATH);

    RS::VMap vmap;
    vmap["Version"] = 1;
    vmap["Display"]["Width"] = 1920;
    vmap["Array"] = { 1, 2, 3, 4 };

    std::optional<std::string> errorMsg;
    RS::VMap::WriteToDisk(vmap, "./Assets/Config/PresistentData.cfg", errorMsg);
    vmap.Clear();
    vmap = RS::VMap::ReadFromDisk("./Assets/Config/PresistentData.cfg", errorMsg);

    int version = vmap["Version"];
    int width = vmap["Display/Width"];
    RS::VArray arr = vmap["Array"];
    auto data = arr[1];

    RS::DisplayDescription displayDesc = {};
    displayDesc.Title                   = RS::Config::Map.Fetch("Display/InitialState/Title", "Editor");
    displayDesc.Width                   = RS::Config::Map.Fetch("Display/InitialState/DefaultWidth", 1920u);
    displayDesc.Height                  = RS::Config::Map.Fetch("Display/InitialState/DefaultHeight", 1080u);
    displayDesc.Fullscreen              = RS::Config::Map.Fetch("Display/InitialState/Fullscreen", false);
    displayDesc.UseWindowedFullscreen   = RS::Config::Map.Fetch("Display/Settings/WindowedFullscreen", false);
    displayDesc.VSync                   = RS::Config::Map.Fetch("Display/Settings/VSync", true);
    RS::Display::Get()->Init(displayDesc);
    RS::Input::Get()->Init();

    RS::ImGuiRenderer::Get()->additionalResizeFunction = [](uint32 width, uint32 height) { RSE::Editor::Get()->Resize(width, height); };
    auto pEngineLook = RS::EngineLoop::Get();
    pEngineLook->additionalFixedTickFunction = []() { RSE::Editor::Get()->FixedUpdate(); };
    pEngineLook->additionalTickFunction = [](const RS::FrameStats& frameStats)
        {
            RSE::Editor::Get()->Update();
            RS::ImGuiRenderer::Get()->Draw([&]()
            {
                RSE::Editor::Get()->Render();
            }
        );
    };

    pEngineLook->Init();
    RSE::Editor::Get()->Init();
    RS::Display::Get()->SetOnSizeChangeCallback(dynamic_cast<RS::IDisplaySizeChange*>(pEngineLook.get()));
    pEngineLook->Run();

    RSE::Editor::Get()->Release();
    pEngineLook->Release();

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
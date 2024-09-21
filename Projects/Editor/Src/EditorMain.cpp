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
    vmap.WriteToDisk("./Assets/Config/PresistentData.cfg", errorMsg);
    vmap.Clear();
    vmap = RS::VMap::ReadFromDisk("./Assets/Config/PresistentData.cfg", errorMsg);

    int version = vmap["Version"];
    int width = vmap["Display/Width"];
    RS::VArray arr = vmap["Array"];
    auto data = arr[1];

    RS::DisplayDescription displayDesc = {};
    displayDesc.Title                   = RS::Config::Get()->Fetch<std::string>("Display/InitialState/Title", "Editor");
    displayDesc.Width                   = RS::Config::Get()->Fetch<uint32>("Display/InitialState/DefaultWidth", 1920);
    displayDesc.Height                  = RS::Config::Get()->Fetch<uint32>("Display/InitialState/DefaultHeight", 1080);
    displayDesc.Fullscreen              = RS::Config::Get()->Fetch<bool>("Display/InitialState/Fullscreen", false);
    displayDesc.UseWindowedFullscreen   = RS::Config::Get()->Fetch<bool>("Display/Settings/WindowedFullscreen", false);
    displayDesc.VSync                   = RS::Config::Get()->Fetch<bool>("Display/Settings/VSync", true);
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
    RS::Display::Get()->Release();
    RS::LaunchArguments::Release();
    return 0;
}
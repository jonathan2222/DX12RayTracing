#include <iostream>

#include <RSEngine.h>
#include "Game1App.h"

#include "Utils/EnumDefines.h"

#include "DX12/Final/DXCore.h"
#include "Core/Console.h"
#include "DX12/Final/DXDisplay.h"

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

    RS::DX12::DXCore::Init();
    std::shared_ptr<RS::Display> pDisplay = RS::Display::Get();

    RS::DX12::DXColorBuffer buffer(RS::Color::ToColor32(1.f, 0.f, 0.f, 1.f));
    buffer.Create(L"Main Color Buffer", pDisplay->GetWidth(), pDisplay->GetHeight(), 1, DXGI_FORMAT_R8G8B8A8_UNORM);



    RS::Console::Get()->Init();
    RS::Input::Get()->AlwaysListenToKey(RS::Key::MICRO); // For console.
    RS::FrameStats frameStats;
    RS::FrameTimer frameTimer;
    frameTimer.Init(&frameStats, 0.25f);
    while (!pDisplay->ShouldClose())
    {
        frameTimer.Begin();

        //m_RenderDoc.StartFrameCapture();

        pDisplay->PollEvents();
        RS::Input::Get()->PreUpdate();

        // Quit by pressing ESCAPE
        {
            if (RS::Input::Get()->IsKeyPressed(RS::Key::ESCAPE))
                pDisplay->Close();
        }

        // Toggle fullscreen by pressing F11
        //{
        //    if (RS::Input::Get()->IsKeyClicked(RS::Key::F11))
        //        pDisplay->ToggleFullscreen();
        //}

        // Toggle console by pressing §
        {
            if (RS::Input::Get()->IsKeyClicked(RS::Key::MICRO))
            {
                RS::Console* pConsole = RS::Console::Get();
                if (pConsole->IsEnabled())
                    pConsole->Disable();
                else
                    pConsole->Enable();
            }
        }

        //frameTimer.FixedTick([&]()
        //    {
        //        FixedTick();
        //        // TODO: Remove this when in Relase build!
        //        m_DebugWindowsManager.FixedTick();
        //    });

        RS::DX12::DXCore::GetDXDisplay()->Present(&buffer);

        RS::Input::Get()->PostUpdate(frameStats.frame.currentDT);

        //m_RenderDoc.EndFrameCapture();

        frameTimer.End();
    }
    buffer.Destroy();
    RS::Console::Get()->Release();
    RS::DX12::DXCore::Release();

    //auto pEngineLoop = RS::EngineLoop::Get();
    //pEngineLoop->Init();
    //{
    //    Game1App app; // Need to be here such that it calls descructor before we release the engine loop.
    //    pEngineLoop->additionalFixedTickFunction = [&]() { app.FixedTick(); };
    //    pEngineLoop->additionalTickFunction = [&](const RS::FrameStats& frameStats) { app.Tick(frameStats); };
    //    RS::Display::Get()->AddOnSizeChangeCallback("EngineLoop SizeChangeCallback", dynamic_cast<RS::IDisplaySizeChange*>(pEngineLoop.get()));
    //    pEngineLoop->Run();
    //}
    //pEngineLoop->Release();

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

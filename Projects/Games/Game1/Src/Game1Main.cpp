#include <iostream>

#include <RSEngine.h>
#include "Game1App.h"

#include "Utils/EnumDefines.h"

#include "DX12/Final/DXCore.h"
#include "Core/Console.h"
#include "DX12/Final/DXDisplay.h"

#include "DX12/Final/DXTexture.h"
#include "Core/CorePlatform.h"
#include "DX12/Final/DXShader.h"
#include "Graphics/RenderCore.h"
#include "DX12/Final/DXCommandContext.h"

#include "Render/ImGuiRenderer.h"

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

    RS::DX12::DXShader::TypeFlag flag = 1;
    flag = flag << 1;

    RS::DX12::DXCore::Init();
    std::shared_ptr<RS::Display> pDisplay = RS::Display::Get();

    RS::DX12::DXColorBuffer buffer(RS::Color::ToColor32(1.f, 0.f, 0.f, 1.f));
    buffer.Create(L"Main Color Buffer", pDisplay->GetWidth(), pDisplay->GetHeight(), 1, DXGI_FORMAT_R8G8B8A8_UNORM);

    RS::ImGuiRenderer::Get()->Init();

    auto pImage = RS::CorePlatform::LoadImageData("flyToYourDream.jpg", RS::Format::RS_FORMAT_R8G8B8A8_UNORM, RS::CorePlatform::ImageFlag::FLIP_Y, true);
    RS::DX12::DXTexture texture;
    texture.Create2D(pImage->width * 4, pImage->width, pImage->height, DXGI_FORMAT_R8G8B8A8_UNORM, pImage->pData);

    RS::DX12::DXGraphicsPSO graphicsPSO;
    {
        using namespace RS::DX12;
        DXShader::Description shaderDesc{};
        shaderDesc.isInternalPath = true;
        shaderDesc.path = "Core/TestShaderPS.hlsl";
        shaderDesc.typeFlags = DXShader::TypeFlag::Auto;
        shaderDesc.defines = { };
        DXShader shader;
        shader.Create(shaderDesc);

        graphicsPSO.SetRootSignature(RS::RenderCore::GetCommonRootSignature());
        graphicsPSO.SetRasterizerState(RS::RenderCore::RasterizerTwoSided);
        graphicsPSO.SetBlendState(RS::RenderCore::BlendDisable);
        graphicsPSO.SetDepthStencilState(RS::RenderCore::DepthStateDisabled);
        graphicsPSO.SetSampleMask(0xFFFFFFFF);
        graphicsPSO.SetInputLayout(0, nullptr);
        graphicsPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        graphicsPSO.SetShader(shader);
        graphicsPSO.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN);
        graphicsPSO.Finalize(false);
        shader.Release();
    }

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

        RS::DX12::DXGraphicsContext& context = RS::DX12::DXGraphicsContext::Begin(L"Color");

        context.SetRootSignature(RS::RenderCore::GetCommonRootSignature());
        context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        context.SetPipelineState(graphicsPSO);
        context.SetConstant(0, 0, RS::DX12::DXDWParam(1.0f));
        context.TransitionResource(texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        context.SetDynamicDescriptor(1, 0, texture.GetSRV());
        context.TransitionResource(buffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
        context.SetRenderTarget(buffer.GetRTV());
        context.SetViewportAndScissor(0, 0, buffer.GetWidth(), buffer.GetHeight());
        context.Draw(3);

        if (RS::Console::Get()->IsEnabled() || ImGui::HasToastNotifications())
        {
            RS::ImGuiRenderer::Get()->Draw([&]() {
                RS::Console::Get()->Render();

                // Render toasts on top of everything, at the end of your code!
                // You should push style vars here
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f); // Round borders
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f)); // Background color
                ImGui::RenderNotifications(); // <-- Here we render all notifications
                ImGui::PopStyleVar(1); // Don't forget to Pop()
                ImGui::PopStyleColor(1);
            });
        }
        RS::ImGuiRenderer::Get()->Render(buffer, &context);

        RS::DX12::DXCore::GetDXDisplay()->Present(&buffer, &context);

        RS::Input::Get()->PostUpdate(frameStats.frame.currentDT);

        //m_RenderDoc.EndFrameCapture();

        frameTimer.End();
    }

    RS::ImGuiRenderer::Get()->Release();
    buffer.Destroy();
    RS::Console::Get()->Release();

    texture.Destroy();
    RS::DX12::DXCore::Destroy();

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

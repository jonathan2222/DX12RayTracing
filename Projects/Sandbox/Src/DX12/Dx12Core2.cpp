#include "PreCompiled.h"
#include "Dx12Core2.h"

#include "Core/Display.h"

#include "DX12/Shader.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb_image.h>

#include "Render/ImGuiRenderer.h"

RS::DX12::Dx12Core2* RS::DX12::Dx12Core2::Get()
{
    static std::unique_ptr<RS::DX12::Dx12Core2> pCore{ std::make_unique<RS::DX12::Dx12Core2>() };
#ifdef RS_CONFIG_DEBUG
    RS_ASSERT(pCore.get()->m_IsReleased == false, "Trying to access a released Dx12 Core!");
#endif
    return pCore.get();
}

void RS::DX12::Dx12Core2::Init(HWND window, int width, int height)
{
    D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    DXGIFlags dxgiFlags = DXGIFlag::REQUIRE_TEARING_SUPPORT;
    m_Device.Init(d3dFeatureLevel, dxgiFlags);

    m_FrameCommandList.Init(m_Device.GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_DescriptorHeapRTV.Init(512, false, "RTV Descriptor Heap");
    m_DescriptorHeapDSV.Init(512, false, "DSV Descriptor Heap");
    m_DescriptorHeapGPUResources.Init(4096, true, "CBV/SRV/UAV Descriptor Heap");
    m_DescriptorHeapSamplers.Init(128, true, "Sampler Descriptor Heap");

    m_IsReleased = false;

    // TODO: Move these outside of Dx12Core2!
    { // Dx12Core2 users
        m_IsWindowVisible = true;
        m_Surface.Init(window, width, height, dxgiFlags);

        // ImGui
        ImGuiRenderer::Get()->Init();

        m_Pipeline.Init();

        struct Vertex
        {
            float position[3];
            float color[4];
            float uv[2];
        };

        float scale = 0.25f;
        float aspectRatio = (float)Display::Get()->GetWidth() / Display::Get()->GetHeight();
        Vertex triangleVertices[] =
        {
            { { -scale, +scale * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } }, // TL
            { { +scale, +scale * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }, // TR
            { { -scale, -scale * aspectRatio, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } }, // BL

            { { +scale, +scale * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }, // TR
            { { +scale, -scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } }, // BR
            { { -scale, -scale * aspectRatio, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } }  // BL
        };
 
        const UINT vertexBufferSize = sizeof(triangleVertices);
        m_VertexBuffer.Create((uint8*)&triangleVertices[0], sizeof(Vertex), vertexBufferSize);

        const uint32 size = Display::Get()->GetWidth()* Display::Get()->GetHeight() * 4;
        struct ConstantBufferData
        {
            float miscData[4] = { 1.0, 1.0, 1.0, 1.0 };
        } textIndex;
        
        m_ConstantBuffer.Create((uint8*)&textIndex, sizeof(textIndex));
        m_ConstantBuffer.CreateView();

        // Load image
        {
            std::string texturePath = RS_TEXTURE_PATH +std::string("flyToYourDream.jpg");
            int w, h, n;
            const int requestedChannelCount = 4;
            uint8* data = stbi_load(texturePath.c_str(), &w, &h, &n, requestedChannelCount);
            RS_ASSERT(data, "Could not load texture! Reason: {}", stbi_failure_reason());
            { // Flip image in y
                std::vector<uint8> buffer(w * requestedChannelCount);
                for (int y = 0; y < h/2; y++)
                {
                    int top = y * w * requestedChannelCount;
                    int bot = (h - y - 1) * w * requestedChannelCount;
                    std::swap_ranges(data + top, data + top + buffer.size(), data + bot);
                }
            }
            m_FrameCommandList.BeginFrame(nullptr);
            m_Texture.Create(data, w, h, GetDXGIFormat(requestedChannelCount ? requestedChannelCount : n, 8, FormatType::UNORM), "FlyToYTourDeam Texture Resource");
            m_FrameCommandList.EndFrame();
            m_Texture.CreateView();
            stbi_image_free(data);
            data = nullptr;
        }

        // Load Null image
        {
            std::string texturePath = RS_TEXTURE_PATH + std::string("NullTexture.png");
            int w, h, n;
            const int requestedChannelCount = 4;
            uint8* data = stbi_load(texturePath.c_str(), &w, &h, &n, requestedChannelCount);
            RS_ASSERT(data, "Could not load texture! Reason: {}", stbi_failure_reason());
            { // Flip image in y
                std::vector<uint8> buffer(w * requestedChannelCount);
                for (int y = 0; y < h / 2; y++)
                {
                    int top = y * w * requestedChannelCount;
                    int bot = (h - y - 1) * w * requestedChannelCount;
                    std::swap_ranges(data + top, data + top + buffer.size(), data + bot);
                }
            }
            m_FrameCommandList.BeginFrame(nullptr);
            m_NullTexture.Create(data, w, h, GetDXGIFormat(requestedChannelCount ? requestedChannelCount : n, 8, FormatType::UNORM), "Null Texture Resource");
            m_FrameCommandList.EndFrame();
            m_NullTexture.CreateView();
            stbi_image_free(data);
            data = nullptr;
        }

        m_FrameCommandList.WaitForGPUQueue();
    }

    // ---------- Tests ----------
    //{
    //    Shader shader;
    //    Shader::Description shaderDesc;
    //    shaderDesc.path = "TestFolder";
    //    shaderDesc.typeFlags = Shader::TypeFlag::Auto;
    //    shader.Create(shaderDesc);
    //    shader.Release();
    //}
    //
    //{
    //    Shader shader;
    //    Shader::Description shaderDesc;
    //    shaderDesc.path = "TestFolder2";
    //    shaderDesc.typeFlags = Shader::TypeFlag::Vertex | Shader::TypeFlag::Pixel;
    //    shader.Create(shaderDesc);
    //    shader.Release();
    //}
    //
    //{
    //    Shader shader;
    //    Shader::Description shaderDesc;
    //    shaderDesc.path = "testName";
    //    shaderDesc.typeFlags = Shader::TypeFlag::Geometry;
    //    shader.Create(shaderDesc);
    //    shader.Release();
    //}
    //
    //{
    //    Shader shader;
    //    Shader::Description shaderDesc;
    //    shaderDesc.path = "testName";
    //    shaderDesc.typeFlags = Shader::TypeFlag::Pixel | Shader::TypeFlag::Vertex;
    //    shader.Create(shaderDesc);
    //    shader.Release();
    //}
    //
    //{
    //    Shader shader;
    //    Shader::Description shaderDesc;
    //    shaderDesc.path = "tmpShaders.hlsl";
    //    shaderDesc.typeFlags = Shader::TypeFlag::Auto;
    //    shader.Create(shaderDesc);
    //    shader.Release();
    //}
    //
    //{
    //    Shader shader;
    //    Shader::Description shaderDesc;
    //    shaderDesc.path = "tmpShaders.hlsl";
    //    shaderDesc.typeFlags = Shader::TypeFlag::Compute;
    //    shader.Create(shaderDesc);
    //    shader.Release();
    //}
    //
    //{
    //    Shader shader;
    //    Shader::Description shaderDesc;
    //    shaderDesc.path = "tmpShaders.hlsl";
    //    shaderDesc.typeFlags = Shader::TypeFlag::Pixel;
    //    shader.Create(shaderDesc);
    //    shader.Release();
    //}
    //
    //{
    //    Shader shader;
    //    Shader::Description shaderDesc;
    //    shaderDesc.path = "tmpShaders.hlsl";
    //    shaderDesc.typeFlags = Shader::TypeFlag::Pixel | Shader::TypeFlag::Vertex | Shader::TypeFlag::Geometry;
    //    shader.Create(shaderDesc);
    //    shader.Release();
    //}
    //
    //{
    //    Shader shader;
    //    Shader::Description shaderDesc;
    //    shaderDesc.path = "customEntryPoints.hlsl";
    //    shaderDesc.typeFlags = Shader::TypeFlag::Auto;
    //    shaderDesc.customEntryPoints.push_back({ Shader::TypeFlag::Pixel, "PSMain" });
    //    shaderDesc.customEntryPoints.push_back({ Shader::TypeFlag::Vertex, "VSMain" });
    //    shader.Create(shaderDesc);
    //    shader.Release();
    //}
    //
    //{
    //    Shader shader;
    //    Shader shaderVertex;
    //    Shader::Description shaderDesc;
    //    shaderDesc.path = "tmpShaders.hlsl";
    //    shaderDesc.typeFlags = Shader::TypeFlag::Pixel;
    //    shader.Create(shaderDesc);
    //
    //    shaderDesc.typeFlags = Shader::TypeFlag::Vertex;
    //    shaderVertex.Create(shaderDesc);
    //
    //    shader.Combine(shaderVertex);
    //    shader.Release();
    //}
}

void RS::DX12::Dx12Core2::Release()
{
    // TODO: Move these outside of Dx12Core2!
    { // Dx12Core2 users
        m_NullTexture.Release();
        m_Texture.Release();
        m_ConstantBuffer.Release();

        // ImGui
        ImGuiRenderer::Get()->FreeDescriptor();

        m_VertexBuffer.Release();
        m_Pipeline.Release();
        m_Surface.ReleaseResources();
    }

    m_FrameCommandList.Release();

    // NOTE: We don't call ProcessDeferredReleases at the end because some resources (such as swap chain) can't be released before their depending resources are released.
    ReleaseDeferredResources();

    m_DescriptorHeapRTV.Release();
    m_DescriptorHeapDSV.Release();
    m_DescriptorHeapGPUResources.Release();
    m_DescriptorHeapSamplers.Release();

    // NOTE: Some types only use deferred release for their resources during shutown/reset/clear. To finally release these resources we call ProcessDeferredReleases once more.
    ProcessDeferredReleases(GetCurrentFrameIndex());

    // ImGui
    ImGuiRenderer::Get()->Release();

    m_Surface.Release();
    m_IsWindowVisible = false;

    m_IsReleased = true;
    m_Device.Release();

#ifdef RS_CONFIG_DEBUG
    DX12::ReportLiveObjects();
#endif
}

bool RS::DX12::Dx12Core2::WindowSizeChanged(uint32 width, uint32 height, bool isFullscreen, bool windowed, bool minimized)
{
    m_IsWindowVisible = width != 0 && height != 0 && !minimized;
    if (!m_IsWindowVisible)
    {
        return false;
    }
    LOG_WARNING("Resize to: ({}, {}) isFullscreen: {}, windowed: {}", width, height, isFullscreen, windowed);

    m_FrameCommandList.Flush();
    m_FrameCommandList.WaitForGPUQueue();
    m_Surface.Resize(width, height, isFullscreen && !windowed);
    ImGuiRenderer::Get()->Resize();

    return true;
}

void RS::DX12::Dx12Core2::Render()
{
    // Wait for the current frame's commands to finish, then resets both the command list and the command allocator.
    m_FrameCommandList.BeginFrame(m_Pipeline.GetPipelineState());

    const uint32 frameIndex = GetCurrentFrameIndex();
    if (m_DeferredReleasesFlags[frameIndex])
    {
        ProcessDeferredReleases(frameIndex);
    }

    ID3D12GraphicsCommandList* pCommandList = m_FrameCommandList.GetCommandList();
    m_Surface.PrepareDraw(pCommandList);

    // Record commands...
    {
        const Dx12Surface::RenderTarget& rt = m_Surface.GetCurrentRenderTarget();

        pCommandList->SetGraphicsRootSignature(m_Pipeline.GetRootSignature());
        pCommandList->SetPipelineState(m_Pipeline.GetPipelineState());
        D3D12_VIEWPORT viewport{};
        viewport.MaxDepth = 0;
        viewport.MinDepth = 1;
        viewport.TopLeftX = viewport.TopLeftY = 0;
        viewport.Width = Display::Get()->GetWidth();
        viewport.Height = Display::Get()->GetHeight();
        pCommandList->RSSetViewports(1, &viewport);
        D3D12_RECT scissorRect{};
        scissorRect.left = scissorRect.top = 0;
        scissorRect.right = viewport.Width;
        scissorRect.bottom = viewport.Height;
        pCommandList->RSSetScissorRects(1, &scissorRect);

        pCommandList->OMSetRenderTargets(1, &rt.handle.m_Cpu, false, nullptr);

        const FLOAT clearColor[4] = { 0.1, 0.7, 0.3, 1.0 };
        D3D12_RECT rect = {};
        rect.left = rect.top = 0;
        rect.right = m_Surface.GetWidth();
        rect.bottom = m_Surface.GetHeight();
        pCommandList->ClearRenderTargetView(rt.handle.m_Cpu, clearColor, 1, &rect);

        pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pCommandList->IASetVertexBuffers(0, 1, &m_VertexBuffer.view);

        struct RootConstData
        {
            const float tint[4] = { 1.0, 1.0, 1.0, 1.0 };
            uint32 texIndex[4] = { (uint32)-1, (uint32)-1, (uint32)-1, (uint32)-1};
        } rootConstData;
        rootConstData.texIndex[0] = m_Texture.GetDescriptorIndex();
        rootConstData.texIndex[1] = rootConstData.texIndex[2] = rootConstData.texIndex[3] = m_NullTexture.GetDescriptorIndex();
        pCommandList->SetGraphicsRoot32BitConstants(0, 4*2, &rootConstData, 0);

        ID3D12DescriptorHeap* pHeaps[2] = { m_DescriptorHeapGPUResources.GetHeap(), m_DescriptorHeapSamplers.GetHeap()};
        pCommandList->SetDescriptorHeaps(2, pHeaps); // Max 2 types of heaps can be bound (only CBV/SRV/UAV and Sampler)

        pCommandList->SetGraphicsRootConstantBufferView(1, m_ConstantBuffer.pUploadHeap->GetGPUVirtualAddress());
        
        // Set heap pointer for Bindless resources.
        pCommandList->SetGraphicsRootDescriptorTable(2, m_DescriptorHeapGPUResources.GetGpuStart());

        //pCommandList->SetComputeRootDescriptorTable(2, );

        pCommandList->DrawInstanced(6, 1, 0, 0);
    }

    static bool show_demo_window = true;
    static bool show_test = true;
    static uint32_t position = ImGuiToastPos_BottomRight;
    ImGuiRenderer::Get()->Draw([&]() {
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        if (show_test)
        {
            ImGui::Begin("Dear ImGui Test", &show_test);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), u8"\uF06A"); // Warning
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), u8"\uF057"); // Error
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), u8"\uF058"); // Success
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), u8"\uF059"); // Info

            if (ImGui::Button("Warning"))
                ImGui::InsertNotification({ ImGuiToastType_Warning, 3000, "Hello World! This is a warning! {}", 0x1337 });
            if (ImGui::Button("Success"))
                ImGui::InsertNotification({ ImGuiToastType_Success, 3000, "Hello World! This is a success! {}", "We can also format here:)" });
            if (ImGui::Button("Error"))
                ImGui::InsertNotification({ ImGuiToastType_Error, 3000, "Hello World! This is an error! {:#10X}", 0xDEADBEEF });
            if (ImGui::Button("Info"))
                ImGui::InsertNotification({ ImGuiToastType_Info, 3000, "Hello World! This is an info!" });
            if (ImGui::Button("Info Long"))
                ImGui::InsertNotification({ ImGuiToastType_Info, 3000, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation" });

            if (ImGui::Button("Custom title"))
            {
                // Now using a custom title...
                ImGuiToast toast(ImGuiToastType_Success, 3000); // <-- content can also be passed here as above
                toast.set_title("This is a {} title", "wonderful");
                toast.set_content("Lorem ipsum dolor sit amet");
                ImGui::InsertNotification(toast);
            }

            if (ImGui::Button("No dismiss"))
            {
                ImGui::InsertNotification({ ImGuiToastType_Error, NOTIFY_NO_DISMISS, "Test 0x%X", 0xDEADBEEF }).bind([]()
                    {
                        ImGui::InsertNotification({ ImGuiToastType_Info, "Signaled" });
                    }
                ).dismissOnSignal();
            }

            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::MenuItem("Top-left", NULL, position == ImGuiToastPos_TopLeft)) { position = ImGuiToastPos_TopLeft; ImGui::SetToastPosition(position); }
                if (ImGui::MenuItem("Top-Center", NULL, position == ImGuiToastPos_TopCenter)) { position = ImGuiToastPos_TopCenter; ImGui::SetToastPosition(position); }
                if (ImGui::MenuItem("Top-right", NULL, position == ImGuiToastPos_TopRight)) { position = ImGuiToastPos_TopRight; ImGui::SetToastPosition(position); }
                if (ImGui::MenuItem("Bottom-left", NULL, position == ImGuiToastPos_BottomLeft)) { position = ImGuiToastPos_BottomLeft; ImGui::SetToastPosition(position); }
                if (ImGui::MenuItem("Bottom-Center", NULL, position == ImGuiToastPos_BottomCenter)) { position = ImGuiToastPos_BottomCenter; ImGui::SetToastPosition(position); }
                if (ImGui::MenuItem("Bottom-right", NULL, position == ImGuiToastPos_BottomRight)) { position = ImGuiToastPos_BottomRight; ImGui::SetToastPosition(position); }
                ImGui::EndPopup();
            }

            ImGui::End();
        }

        // Render toasts on top of everything, at the end of your code!
        // You should push style vars here
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f); // Round borders
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f)); // Background color
        ImGui::RenderNotifications(); // <-- Here we render all notifications
        ImGui::PopStyleVar(1); // Don't forget to Pop()
        ImGui::PopStyleColor(1);
    });

    // ImGui
    ImGuiRenderer::Get()->Render();

    m_Surface.EndDraw(pCommandList);

    // Tell the GPU to exectue the command lists, appened signal command and go to the next frame.
    m_FrameCommandList.EndFrame();

    if (Display::Get()->HasFocus())
        m_Surface.Present();

    m_FrameCommandList.MoveToNextFrame();
}

void RS::DX12::Dx12Core2::Transition()
{
}

void RS::DX12::Dx12Core2::ReleaseDeferredResources()
{
    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; ++i)
    {
        m_DeferredReleasesFlags[i] = 0;
        ProcessDeferredReleases(i);
    }
}

void RS::DX12::Dx12Core2::ProcessDeferredReleases(uint32 frameIndex)
{
    std::lock_guard lock{ m_DeferredReleasesMutex };

    // NOTE: We clear this flag in the beginning. If we'd clear it at the end then it might overwrite some other thread that was trying to set it.
    //      It's fine if overwriting happens before processing the items.
    m_DeferredReleasesFlags[frameIndex] = 0;

    m_DescriptorHeapRTV.ProcessDeferredFree(frameIndex);
    m_DescriptorHeapDSV.ProcessDeferredFree(frameIndex);
    m_DescriptorHeapGPUResources.ProcessDeferredFree(frameIndex);
    m_DescriptorHeapSamplers.ProcessDeferredFree(frameIndex);

    std::vector<IUnknown*>& resources = m_DeferredReleases[frameIndex];
    if (!resources.empty())
    {
        for (auto& resource : resources)
            DX12_RELEASE(resource);
        resources.clear();
    }
}

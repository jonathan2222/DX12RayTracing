#include "PreCompiled.h"
#include "Dx12Core2.h"

#include "Core/Display.h"

#include "DX12/Shader.h"

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
    m_DescriptorHeapSRV.Init(4096, true, "SRV Descriptor Heap");
    m_DescriptorHeapUAV.Init(512, false, "UAV Descriptor Heap");

    m_IsReleased = false;

    // TODO: Move these outside of Dx12Core2!
    { // Dx12Core2 users
        m_IsWindowVisible = true;
        m_Surface.Init(window, width, height, dxgiFlags);

        m_Pipeline.Init();

        struct Vertex
        {
            float position[4];
            float color[4];
        };

        float aspectRatio = (float)Display::Get()->GetWidth() / Display::Get()->GetHeight();
        Vertex triangleVertices[] =
        {
            { { 0.0f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };
    
        const UINT vertexBufferSize = sizeof(triangleVertices);
        m_VertexBuffer.Create((uint8*)&triangleVertices[0], sizeof(Vertex), vertexBufferSize);
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
        m_VertexBuffer.Release();
        m_Pipeline.Release();
        m_Surface.ReleaseResources();
    }

    m_FrameCommandList.Release();

    // NOTE: We don't call ProcessDeferredReleases at the end because some resources (such as swap chain) can't be released before their depending resources are released.
    ReleaseDeferredResources();

    m_DescriptorHeapRTV.Release();
    m_DescriptorHeapDSV.Release();
    m_DescriptorHeapSRV.Release();
    m_DescriptorHeapUAV.Release();

    // NOTE: Some types only use deferred release for their resources during shutown/reset/clear. To finally release these resources we call ProcessDeferredReleases once more.
    ProcessDeferredReleases(GetCurrentFrameIndex());

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
        D3D12_VIEWPORT viewport{};
        viewport.MaxDepth = 1;
        viewport.MinDepth = 0;
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
        pCommandList->DrawInstanced(3, 1, 0, 0);
    }

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
    m_DescriptorHeapSRV.ProcessDeferredFree(frameIndex);
    m_DescriptorHeapUAV.ProcessDeferredFree(frameIndex);

    std::vector<IUnknown*>& resources = m_DeferredReleases[frameIndex];
    if (!resources.empty())
    {
        for (auto& resource : resources)
            DX12_RELEASE(resource);
        resources.clear();
    }
}

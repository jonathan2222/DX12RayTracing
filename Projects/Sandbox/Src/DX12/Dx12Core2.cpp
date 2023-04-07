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
    }

    {
        LOG_INFO("Compiling with AUTO, reading folder 'TestFolder'");
        Shader shader;
        Shader::Description shaderDesc;
        shaderDesc.path = "TestFolder";
        shaderDesc.typeFlags = Shader::TypeFlag::AUTO;
        shader.Create(shaderDesc);
        shader.Release();
    }

    {
        LOG_INFO("Compiling with PIXEL | VERTEX, reading folder 'TestFolder2'");
        Shader shader;
        Shader::Description shaderDesc;
        shaderDesc.path = "TestFolder2";
        shaderDesc.typeFlags = Shader::TypeFlag::VERTEX | Shader::TypeFlag::PIXEL;
        shader.Create(shaderDesc);
        shader.Release();
    }

    {
        LOG_INFO("Compiling with GEOMETRY, reading 'testName'");
        Shader shader;
        Shader::Description shaderDesc;
        shaderDesc.path = "testName";
        shaderDesc.typeFlags = Shader::TypeFlag::GEOMETRY;
        shader.Create(shaderDesc);
        shader.Release();
    }

    {
        LOG_INFO("Compiling with PIXEL | VERTEX, reading 'testName'");
        Shader shader;
        Shader::Description shaderDesc;
        shaderDesc.path = "testName";
        shaderDesc.typeFlags = Shader::TypeFlag::PIXEL | Shader::TypeFlag::VERTEX;
        shader.Create(shaderDesc);
        shader.Release();
    }

    LOG_INFO("Compiling with AUTO, reading 'tmpShaders.hlsl'");
    Shader shader1;
    Shader::Description shaderDesc1;
    shaderDesc1.path = "tmpShaders.hlsl";
    shaderDesc1.typeFlags = Shader::TypeFlag::AUTO;
    shader1.Create(shaderDesc1);
    shader1.Release();

    LOG_INFO("Compiling with COMPUTE, reading 'tmpShaders.hlsl'");
    Shader shader2;
    Shader::Description shaderDesc2;
    shaderDesc2.path = "tmpShaders.hlsl";
    shaderDesc2.typeFlags = Shader::TypeFlag::COMPUTE;
    shader2.Create(shaderDesc2);
    shader2.Release();

    LOG_INFO("Compiling with PIXEL, reading 'tmpShaders.hlsl'");
    Shader shader3;
    Shader::Description shaderDesc3;
    shaderDesc3.path = "tmpShaders.hlsl";
    shaderDesc3.typeFlags = Shader::TypeFlag::PIXEL;
    shader3.Create(shaderDesc3);
    shader3.Release();

    LOG_INFO("Compiling with PIXEL | VERTEX | GEOMETRY, reading 'tmpShaders.hlsl'");
    Shader shader;
    Shader::Description shaderDesc;
    shaderDesc.path = "tmpShaders.hlsl";
    shaderDesc.typeFlags = Shader::TypeFlag::PIXEL | Shader::TypeFlag::VERTEX | Shader::TypeFlag::GEOMETRY;
    shader.Create(shaderDesc);
    shader.Release();
}

void RS::DX12::Dx12Core2::Release()
{
    // TODO: Move these outside of Dx12Core2!
    { // Dx12Core2 users
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
    m_FrameCommandList.BeginFrame();

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
        const FLOAT clearColor[4] = { 0.1, 0.7, 0.3, 1.0 };
        D3D12_RECT rect = {};
        rect.left = rect.top = 0;
        rect.right = m_Surface.GetWidth();
        rect.bottom = m_Surface.GetHeight();
        pCommandList->ClearRenderTargetView(rt.handle.m_Cpu, clearColor, 1, &rect);
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

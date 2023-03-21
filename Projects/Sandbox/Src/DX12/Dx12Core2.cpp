#include "PreCompiled.h"
#include "Dx12Core2.h"

RS::DX12::Dx12Core2* RS::DX12::Dx12Core2::Get()
{
    static std::unique_ptr<RS::DX12::Dx12Core2> pCore{ std::make_unique<RS::DX12::Dx12Core2>() };
    return pCore.get();
}

void RS::DX12::Dx12Core2::Init(HWND window, int width, int height)
{
    m_Window = window;

    D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    m_Device.Init(d3dFeatureLevel, DXGIFlag::REQUIRE_TEARING_SUPPORT);

    m_FrameCommandList.Init(m_Device.GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_DescriptorHeapRTV.Init(512, false, "RTV Descriptor Heap");
    m_DescriptorHeapDSV.Init(512, false, "DSV Descriptor Heap");
    m_DescriptorHeapSRV.Init(4096, true, "SRV Descriptor Heap");
    m_DescriptorHeapUAV.Init(512, false, "UAV Descriptor Heap");
}

void RS::DX12::Dx12Core2::Release()
{
    m_FrameCommandList.Release();

    // NOTE: We don't call ProcessDeferredReleases at the end because some resources (such as swap chain) can't be released before their depending resources are released.
    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; ++i)
    {
        ProcessDeferredReleases(i);
    }

    m_Device.Release();

    m_DescriptorHeapRTV.Release();
    m_DescriptorHeapDSV.Release();
    m_DescriptorHeapSRV.Release();
    m_DescriptorHeapUAV.Release();

    // NOTE: Some types only use deferred release for their resources during shutown/reset/clear. To finally release these resources we call ProcessDeferredReleases once more.
    ProcessDeferredReleases(0);

#ifdef RS_CONFIG_DEBUG
    DX12::ReportLiveObjects();
#endif
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

    // Record commands...

    // Tell GPU to exectue the command lists, appened signal command and go to the next frame.
    m_FrameCommandList.EndFrame();
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

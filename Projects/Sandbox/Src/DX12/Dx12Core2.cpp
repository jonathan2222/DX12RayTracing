#include "PreCompiled.h"
#include "Dx12Core2.h"

RS::DX12::Dx12Core2* RS::DX12::Dx12Core2::Get()
{
    static std::unique_ptr<RS::DX12::Dx12Core2> pCore{ std::make_unique<RS::DX12::Dx12Core2>() };
    return pCore.get();
}

void RS::DX12::Dx12Core2::Init()
{
    D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    m_Device.Init(d3dFeatureLevel, DXGIFlag::REQUIRE_TEARING_SUPPORT);

    m_FrameCommandList.Init(m_Device.GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_DIRECT);
}

void RS::DX12::Dx12Core2::Release()
{
    m_FrameCommandList.Release();
    m_Device.Release();

#ifdef RS_CONFIG_DEBUG
    DX12::ReportLiveObjects();
#endif
}

void RS::DX12::Dx12Core2::Render()
{
    // Wait for the current frame's commands to finish, then resets both the command list and the command allocator.
    m_FrameCommandList.BeginFrame();

    // Record commands...

    // Tell GPU to exectue the command lists, appened signal command and go to the next frame.
    m_FrameCommandList.EndFrame();
}

#include "PreCompiled.h"
#include "Dx12Surface.h"

#include "Dx12Core2.h"
#include "DX12Defines.h"

void RS::DX12::Dx12Surface::Init(HWND window, uint32 width, uint32 height, DXGIFlags flags)
{
    m_Width = width;
    m_Height = height;
    m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    CreateSwapChain(window, width, height, m_Format, flags);

    Dx12DescriptorHeap* pRtvDescHeap = Dx12Core2::Get()->GetDescriptorHeapRTV();
    CreateResources(m_Format, pRtvDescHeap);
}

void RS::DX12::Dx12Surface::ReleaseResources()
{
    Dx12DescriptorHeap* pRtvDescHeap = Dx12Core2::Get()->GetDescriptorHeapRTV();

    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        RenderTarget& rt = m_RenderTargets[i];
        pRtvDescHeap->Free(rt.handle);
        Dx12Core2::Get()->DeferredRelease(rt.pResource);
    }
}

void RS::DX12::Dx12Surface::Release()
{
	DX12_RELEASE(m_SwapChain);
}

void RS::DX12::Dx12Surface::PrepareDraw(ID3D12GraphicsCommandList* pCommandList)
{
    m_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

    RenderTarget& rt = m_RenderTargets[m_BackBufferIndex];
    D3D12_RESOURCE_STATES newState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    if (rt.state != newState)
    {
        // Transition the render target into the correct state to allow for drawing into it.
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(rt.pResource, rt.state, newState);
        pCommandList->ResourceBarrier(1, &barrier);
        rt.state = newState;
    }
}

void RS::DX12::Dx12Surface::EndDraw(ID3D12GraphicsCommandList* pCommandList)
{
    RenderTarget& rt = m_RenderTargets[m_BackBufferIndex];
    D3D12_RESOURCE_STATES newState = D3D12_RESOURCE_STATE_PRESENT;
    if (rt.state != newState)
    {
        // Transition the render target into the correct state to allow for presenting.
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(rt.pResource, rt.state, newState);
        pCommandList->ResourceBarrier(1, &barrier);
        rt.state = newState;
    }
}

void RS::DX12::Dx12Surface::Present()
{
    HRESULT hr = E_FAIL;
    if (m_Flags & DXGIFlag::ALLOW_TEARING)
    {
        // Recommended to always use tearing if supported when using a sync interval of 0.
        // Note this will fail if in true 'fullscreen' mode.
        hr = m_SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    }
    else
    {
        // The first argument instructs DXGI to block until VSync, putting the application
        // to sleep until the next VSync. This ensures we don't waste any cycles rendering
        // frames that will never be displayed to the screen.
        hr = m_SwapChain->Present(1, 0);
    }

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        DX12_DEVICE_PTR const pDevice = Dx12Core2::Get()->GetD3D12Device();
        LOG_DEBUG("Device Lost on Present: Reason code {:#010x}", (hr == DXGI_ERROR_DEVICE_REMOVED) ? pDevice->GetDeviceRemovedReason() : hr);
        // TODO: Handle device lost!
        RS_ASSERT(false, "Device lost, TODO: Recreate the whole renderer!");
    }
    else
    {
        DXCallVerbose(hr, "Failed to present frame!");
    }
}

void RS::DX12::Dx12Surface::CreateSwapChain(HWND window, uint32 width, uint32 height, DXGI_FORMAT format, DXGIFlags flags)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = format;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Flags = (flags & DXGIFlag::ALLOW_TEARING) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenSwapChainDesc = { 0 };
    fullscreenSwapChainDesc.Windowed = TRUE;

    DX12_FACTORY_PTR const pFactory = Dx12Core2::Get()->GetDXGIFactory();
    RS_ASSERT_NO_MSG(pFactory);
    const Dx12FrameCommandList* pFrameCommandList = Dx12Core2::Get()->GetFrameCommandList();
    RS_ASSERT_NO_MSG(pFrameCommandList);

    ComPtr<IDXGISwapChain1> swapChain;
    DXCall(pFactory->CreateSwapChainForHwnd(pFrameCommandList->GetCommandQueue(), window, &swapChainDesc, &fullscreenSwapChainDesc, nullptr, &swapChain));
    DXCallVerbose(swapChain->QueryInterface(IID_PPV_ARGS(&m_SwapChain)));

    DXCallVerbose(pFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
}

void RS::DX12::Dx12Surface::CreateResources(DXGI_FORMAT format, Dx12DescriptorHeap* pRTVDescHeap)
{
    DX12_DEVICE_PTR const pDevice = Dx12Core2::Get()->GetD3D12Device();
    RS_ASSERT_NO_MSG(pDevice);
    RS_ASSERT_NO_MSG(pRTVDescHeap->GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        RenderTarget& rt = m_RenderTargets[i];
        rt.handle = pRTVDescHeap->Allocate();

        m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&rt.pResource));
        DX12_SET_DEBUG_NAME_REF(rt.pResource, rt.debugName, "Swap Chain RTV[{}]", i);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = format;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        // Use the allocated memory that the handle points to and set the data for a RTV at that position.
        pDevice->CreateRenderTargetView(rt.pResource, &rtvDesc, rt.handle.m_Cpu);
    }
}

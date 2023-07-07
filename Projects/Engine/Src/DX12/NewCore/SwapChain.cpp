#include "PreCompiled.h"
#include "SwapChain.h"

#include "DX12/NewCore/DX12Core3.h"

RS::SwapChain::SwapChain(HWND window, uint32 width, uint32 height, DX12::DXGIFlags flags)
    : m_Width(width)
    , m_Height(height)
    , m_Format(DXGI_FORMAT_R8G8B8A8_UNORM)
    , m_Flags(flags)
    , m_BackBufferIndex(FRAME_BUFFER_COUNT - 1)
    , m_IsFullscreen(false)
    , m_UseVSync(false)
    , m_FenceValues{ 0 }
{
    m_pRenderTareget = std::make_shared<RenderTarget>();

    CreateSwapChain(window, width, height, m_Format, flags);
    CreateResources();
}

RS::SwapChain::~SwapChain()
{
}

uint32 RS::SwapChain::Present(const std::shared_ptr<Texture>& pTexture)
{
    auto pCommandQueue = DX12Core3::Get()->GetDirectCommandQueue();
    auto pCommandList = pCommandQueue->GetCommandList();

    auto pBackBuffer = m_BackBufferTextures[m_BackBufferIndex];

    if (pTexture)
    {
        if (pTexture->GetD3D12ResourceDesc().SampleDesc.Count > 1)
        {
            //pCommandList->ResolveSubresource(backBuffer, texture);
            RS_ASSERT(false, "Does not support multisample!");
        }
        else
        {
            pCommandList->CopyResource(pBackBuffer, pTexture);
        }
    }

    pCommandList->TransitionBarrier(pBackBuffer, D3D12_RESOURCE_STATE_PRESENT);
    pCommandQueue->ExecuteCommandList(pCommandList);

    HRESULT hr = E_FAIL;
    if (m_Flags & DX12::DXGIFlag::ALLOW_TEARING)
    {
        // Recommended to always use tearing if supported when using a sync interval of 0.
        // Note this will fail if in true 'fullscreen' mode.
        hr = m_pSwapChain->Present(m_UseVSync ? 1 : 0, m_IsFullscreen ? 0 : DXGI_PRESENT_ALLOW_TEARING);
    }
    else
    {
        // The first argument instructs DXGI to block until VSync, putting the application
        // to sleep until the next VSync. This ensures we don't waste any cycles rendering
        // frames that will never be displayed to the screen.
        hr = m_pSwapChain->Present(m_UseVSync ? 1 : 0, 0);
    }

    auto pDevice = DX12Core3::Get()->GetD3D12Device();
    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        LOG_DEBUG("Device Lost on Present: Reason code {:#010x}", (hr == DXGI_ERROR_DEVICE_REMOVED) ? pDevice->GetDeviceRemovedReason() : hr);
        // TODO: Handle device lost!
        RS_ASSERT(false, "Device lost, TODO: Recreate the whole renderer!");
    }
    else
    {
        DXCallVerbose(hr, "Failed to present frame!");
    }

    m_FenceValues[m_BackBufferIndex] = pCommandQueue->Signal();

    m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

    uint64 fenceValue = m_FenceValues[m_BackBufferIndex];
    pCommandQueue->WaitForFenceValue(fenceValue);

    DX12Core3::Get()->ReleaseStaleDescriptors();

    return m_BackBufferIndex;
}

void RS::SwapChain::Resize(uint32 width, uint32 height, bool isFullscreen)
{
    if (m_Width != width || m_Height != height)
    {
        m_Width = std::max(1u, width);
        m_Height = std::max(1u, height);

        // Wait on GPU work.
        DX12Core3::Get()->Flush();

        // Remove the previous resources.
        m_pRenderTareget->Reset();
        for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
            m_BackBufferTextures[i].reset();

        // Remove all pending resources.
        DX12Core3::Get()->WaitForGPU();

        // Resize dxgi swap chain back buffers.
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        DXCall(m_pSwapChain->GetDesc(&swapChainDesc));
        DXCall(m_pSwapChain->ResizeBuffers(FRAME_BUFFER_COUNT, m_Width, m_Height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
        
        // Create new frame buffer resources.
        CreateResources();
    }
}

std::shared_ptr<RS::RenderTarget> RS::SwapChain::GetCurrentRenderTarget() const
{
    m_pRenderTareget->SetAttachment(AttachmentPoint::Color0, GetCurrentBackBuffer());
    return m_pRenderTareget;
}

void RS::SwapChain::CreateSwapChain(HWND window, uint32 width, uint32 height, DXGI_FORMAT format, DX12::DXGIFlags flags)
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
    swapChainDesc.Flags = (flags & DX12::DXGIFlag::ALLOW_TEARING) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenSwapChainDesc = { 0 };
    fullscreenSwapChainDesc.Windowed = TRUE;

    auto pFactory = DX12Core3::Get()->GetDXGIFactory();
    RS_ASSERT_NO_MSG(pFactory);

    CommandQueue* pCommandQueue = DX12Core3::Get()->GetDirectCommandQueue();
    RS_ASSERT_NO_MSG(pCommandQueue);

    ComPtr<IDXGISwapChain1> swapChain;
    DXCall(pFactory->CreateSwapChainForHwnd(pCommandQueue->GetD3D12CommandQueue().Get(), window, &swapChainDesc, &fullscreenSwapChainDesc, nullptr, &swapChain));
    DXCallVerbose(swapChain->QueryInterface(IID_PPV_ARGS(&m_pSwapChain)));

    DXCallVerbose(pFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));

    m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}

void RS::SwapChain::CreateResources()
{
    auto pDevice = DX12Core3::Get()->GetD3D12Device();
    RS_ASSERT_NO_MSG(pDevice);

    for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> pBackBufferResource;
        m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBufferResource));

        ResourceStateTracker::AddGlobalResourceState(pBackBufferResource.Get(), D3D12_RESOURCE_STATE_COMMON);

        std::string name = Utils::Format("Swap Chain RTV[{}]", i);
        m_BackBufferTextures[i] = std::make_shared<Texture>(pBackBufferResource, name);
    }
}

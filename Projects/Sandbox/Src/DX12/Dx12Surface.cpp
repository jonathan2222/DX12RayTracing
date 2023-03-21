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
}

void RS::DX12::Dx12Surface::Release()
{
	DX12_RELEASE(m_SwapChain);
	for(uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
		DX12_RELEASE(m_RenderTargets[i]);
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

    IDXGIFactory4* const pFactory = Dx12Core2::Get()->GetDXGIFactory();
    RS_ASSERT_NO_MSG(pFactory);
    const Dx12FrameCommandList* pFrameCommandList = Dx12Core2::Get()->GetFrameCommandList();
    RS_ASSERT_NO_MSG(pFrameCommandList);

    ComPtr<IDXGISwapChain1> swapChain;
    DXCallVerbose(pFactory->CreateSwapChainForHwnd(pFrameCommandList->GetCommandQueue(), window, &swapChainDesc, &fullscreenSwapChainDesc, nullptr, &swapChain));
    DXCall(swapChain->QueryInterface(IID_PPV_ARGS(&m_SwapChain)));
}

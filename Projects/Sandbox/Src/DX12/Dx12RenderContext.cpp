#include "PreCompiled.h"
#include "Dx12RenderContext.h"

RS::DX12::Dx12RenderContext* RS::DX12::Dx12RenderContext::Get()
{
    static std::unique_ptr<RS::DX12::Dx12RenderContext> pContext{ std::make_unique<RS::DX12::Dx12RenderContext>() };
    return pContext.get();
}

void RS::DX12::Dx12RenderContext::BeginFrame()
{
    m_Surface.PrepareDraw(m_FrameCommandList.GetCommandList());
}

void RS::DX12::Dx12RenderContext::EndFrame()
{
    m_Surface.EndDraw(m_FrameCommandList.GetCommandList());
    m_FrameCommandList.EndFrame();
}

void RS::DX12::Dx12RenderContext::Present()
{
    m_Surface.Present();
    m_FrameCommandList.MoveToNextFrame();
}

void RS::DX12::Dx12RenderContext::ClearRTV(const float clearColor[4])
{
    const Dx12Surface::RenderTarget& rt = m_Surface.GetCurrentRenderTarget();
    D3D12_RECT rect = {};
    rect.left = rect.top = 0;
    rect.right = m_Surface.GetWidth();
    rect.bottom = m_Surface.GetHeight();
    m_FrameCommandList.GetCommandList()->ClearRenderTargetView(rt.handle.m_Cpu, clearColor, 1, &rect);
}

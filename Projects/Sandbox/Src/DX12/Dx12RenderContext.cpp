#include "PreCompiled.h"
#include "Dx12RenderContext.h"

RS::DX12::Dx12RenderContext* RS::DX12::Dx12RenderContext::Get()
{
    static std::unique_ptr<RS::DX12::Dx12RenderContext> pContext{ std::make_unique<RS::DX12::Dx12RenderContext>() };
    return pContext.get();
}

void RS::DX12::Dx12RenderContext::BeginFrame()
{
}

void RS::DX12::Dx12RenderContext::EndFrame()
{
}

void RS::DX12::Dx12RenderContext::Present()
{
}

void RS::DX12::Dx12RenderContext::ClearRTV(const float clearColor[4])
{
}

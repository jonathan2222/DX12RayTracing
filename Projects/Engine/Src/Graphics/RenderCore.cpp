#include "PreCompiled.h"
#include "RenderCore.h"

#include "DX12/NewCore/DX12Core3.h"

#include "Graphics/TextRenderer.h"

std::shared_ptr<RS::RenderCore> RS::RenderCore::Get()
{
    static std::shared_ptr<RenderCore> pSelf = std::make_shared<RenderCore>();
    return pSelf;
}

void RS::RenderCore::Init()
{
    auto pCommandQueue = DX12Core3::Get()->GetDirectCommandQueue();
    auto pCommandList = pCommandQueue->GetCommandList();
    
    // Initialize default textures.
    uint8 value = 0u;
    pTextureBlack = pCommandList->CreateTexture(1, 1, (const uint8*)&value, DXGI_FORMAT_R8_UNORM, "Core 1x1 Texture Black");
    value = 255u;
    pTextureWhite = pCommandList->CreateTexture(1, 1, (const uint8*)&value, DXGI_FORMAT_R8_UNORM, "Core 1x1 Texture White");
    
    // Wait for load to finish.
    uint64 fenceValue = pCommandQueue->ExecuteCommandList(pCommandList);
    pCommandQueue->WaitForFenceValue(fenceValue);

    TextRenderer::Get()->Init();
}

void RS::RenderCore::Destory()
{
    TextRenderer::Get()->Destory();

    pTextureBlack.reset();
    pTextureWhite.reset();
}

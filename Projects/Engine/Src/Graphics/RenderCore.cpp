#include "PreCompiled.h"
#include "RenderCore.h"

#include "DX12/Final/DXCore.h"

//#include "Graphics/TextRenderer.h"
//#include "Graphics/MainRootSignature.h"

#include "DX12/Final/DXShader.h"

std::shared_ptr<RS::RenderCore> RS::RenderCore::Get()
{
    static std::shared_ptr<RenderCore> pSelf = std::make_shared<RenderCore>();
    return pSelf;
}

void RS::RenderCore::Init()
{
    //auto pCommandQueue = DX12Core3::Get()->GetDirectCommandQueue();
    //auto pCommandList = pCommandQueue->GetCommandList();
    //
    //// Initialize default textures.
    //uint8 value = 0u;
    //pTextureBlack = pCommandList->CreateTexture(1, 1, (const uint8*)&value, DXGI_FORMAT_R8_UNORM, "Core 1x1 Texture Black");
    //value = 255u;
    //pTextureWhite = pCommandList->CreateTexture(1, 1, (const uint8*)&value, DXGI_FORMAT_R8_UNORM, "Core 1x1 Texture White");
    //
    //// Wait for load to finish.
    //uint64 fenceValue = pCommandQueue->ExecuteCommandList(pCommandList);
    //pCommandQueue->WaitForFenceValue(fenceValue);
    //
    ////MainRootSignature::Get()->Init();
    //
    //TextRenderer::Get()->Init();
}

void RS::RenderCore::Destory()
{
    //TextRenderer::Get()->Destory();

    //MainRootSignature::Get()->Destroy();

    //pTextureBlack.reset();
    //pTextureWhite.reset();
}

//std::shared_ptr<RS::RootSignature> RS::RenderCore::GetMainRootSignature()
//{
//    return MainRootSignature::Get()->m_pRootSignature;
//}

void RS::RenderCore::InitCommonStates()
{
    SamplerLinearWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerLinearWrap = SamplerLinearWrapDesc.CreateDescriptor();

    SamplerLinearClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerLinearClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    SamplerLinearClamp = SamplerLinearClampDesc.CreateDescriptor();

    SamplerPointClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    SamplerPointClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    SamplerPointClamp = SamplerPointClampDesc.CreateDescriptor();

    SamplerLinearBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerLinearBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
    SamplerLinearBorderDesc.SetBorderColor(Color32(0.0f, 0.0f, 0.0f, 0.0f));
    SamplerLinearBorder = SamplerLinearBorderDesc.CreateDescriptor();

    SamplerPointBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    SamplerPointBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
    SamplerPointBorderDesc.SetBorderColor(Color32(0.0f, 0.0f, 0.0f, 0.0f));
    SamplerPointBorder = SamplerPointBorderDesc.CreateDescriptor();

    // Default rasterizer states
    RasterizerDefault.FillMode = D3D12_FILL_MODE_SOLID;
    RasterizerDefault.CullMode = D3D12_CULL_MODE_BACK;
    RasterizerDefault.FrontCounterClockwise = TRUE;
    RasterizerDefault.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    RasterizerDefault.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    RasterizerDefault.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    RasterizerDefault.DepthClipEnable = TRUE;
    RasterizerDefault.MultisampleEnable = FALSE;
    RasterizerDefault.AntialiasedLineEnable = FALSE;
    RasterizerDefault.ForcedSampleCount = 0;
    RasterizerDefault.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    RasterizerDefaultMsaa = RasterizerDefault;
    RasterizerDefaultMsaa.MultisampleEnable = TRUE;

    RasterizerDefaultCw = RasterizerDefault;
    RasterizerDefaultCw.FrontCounterClockwise = FALSE;

    RasterizerDefaultCwMsaa = RasterizerDefaultCw;
    RasterizerDefaultCwMsaa.MultisampleEnable = TRUE;

    RasterizerTwoSided = RasterizerDefault;
    RasterizerTwoSided.CullMode = D3D12_CULL_MODE_NONE;

    RasterizerTwoSidedMsaa = RasterizerTwoSided;
    RasterizerTwoSidedMsaa.MultisampleEnable = TRUE;

    DepthStateDisabled.DepthEnable = FALSE;
    DepthStateDisabled.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    DepthStateDisabled.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    DepthStateDisabled.StencilEnable = FALSE;
    DepthStateDisabled.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    DepthStateDisabled.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    DepthStateDisabled.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    DepthStateDisabled.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    DepthStateDisabled.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    DepthStateDisabled.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    DepthStateDisabled.BackFace = DepthStateDisabled.FrontFace;

    DepthStateReadWrite = DepthStateDisabled;
    DepthStateReadWrite.DepthEnable = TRUE;
    DepthStateReadWrite.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    DepthStateReadWrite.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

    DepthStateReadOnly = DepthStateReadWrite;
    DepthStateReadOnly.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    DepthStateReadOnlyReversed = DepthStateReadOnly;
    DepthStateReadOnlyReversed.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    DepthStateTestEqual = DepthStateReadOnly;
    DepthStateTestEqual.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

    D3D12_BLEND_DESC alphaBlend = {};
    alphaBlend.IndependentBlendEnable = FALSE;
    alphaBlend.RenderTarget[0].BlendEnable = FALSE;
    alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    alphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    alphaBlend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    alphaBlend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    alphaBlend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    alphaBlend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    alphaBlend.RenderTarget[0].RenderTargetWriteMask = 0;
    BlendNoColorWrite = alphaBlend;

    alphaBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    BlendDisable = alphaBlend;

    alphaBlend.RenderTarget[0].BlendEnable = TRUE;
    BlendTraditional = alphaBlend;

    alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    BlendPreMultiplied = alphaBlend;

    alphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
    BlendAdditive = alphaBlend;

    alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    BlendTraditionalAdditive = alphaBlend;

    DispatchIndirectCommandSignature[0].Dispatch();
    DispatchIndirectCommandSignature.Finalize();

    DrawIndirectCommandSignature[0].Draw();
    DrawIndirectCommandSignature.Finalize();

    m_sCommonRootSignature.Reset(4, 3);
    m_sCommonRootSignature[0].InitAsConstants(0, 4);
    m_sCommonRootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10);
    m_sCommonRootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 10);
    m_sCommonRootSignature[3].InitAsConstantBuffer(1);
    m_sCommonRootSignature.InitStaticSampler(0, SamplerLinearClampDesc);
    m_sCommonRootSignature.InitStaticSampler(1, SamplerPointBorderDesc);
    m_sCommonRootSignature.InitStaticSampler(2, SamplerLinearBorderDesc);
    m_sCommonRootSignature.Finalize(L"GraphicsCommonRS");

    using namespace DX12;
    DXShader generateMipsShader;
#define CreatePSO(ObjName, shaderDesc) \
    { \
        generateMipsShader.Create(shaderDesc); \
        ObjName.SetRootSignature(m_sCommonRootSignature); \
        ObjName.SetComputeShader(generateMipsShader); \
        ObjName.Finalize(false); \
        generateMipsShader.Release(); \
    }

    DXShader::Description shaderDesc{};
    shaderDesc.isInternalPath = true;
    shaderDesc.path = "Core/GenerateMipsCS.hlsl";
    shaderDesc.typeFlags = DXShader::TypeFlag::Compute;

    shaderDesc.defines = { };
    CreatePSO(GenerateMipsLinearPSO[0], shaderDesc);
    shaderDesc.defines = { "NON_POWER_OF_TWO 1"};
    CreatePSO(GenerateMipsLinearPSO[1], shaderDesc);
    shaderDesc.defines = { "NON_POWER_OF_TWO 2" };
    CreatePSO(GenerateMipsLinearPSO[2], shaderDesc);
    shaderDesc.defines = { "NON_POWER_OF_TWO 3" };
    CreatePSO(GenerateMipsLinearPSO[3], shaderDesc);

    shaderDesc.defines = { "CONVERT_TO_SRGB" };
    CreatePSO(GenerateMipsGammaPSO[0], shaderDesc);
    shaderDesc.defines = { "CONVERT_TO_SRGB", "NON_POWER_OF_TWO 1" };
    CreatePSO(GenerateMipsGammaPSO[1], shaderDesc);
    shaderDesc.defines = { "CONVERT_TO_SRGB", "NON_POWER_OF_TWO 2" };
    CreatePSO(GenerateMipsGammaPSO[2], shaderDesc);
    shaderDesc.defines = { "CONVERT_TO_SRGB", "NON_POWER_OF_TWO 3" };
    CreatePSO(GenerateMipsGammaPSO[3], shaderDesc);
#undef CreatePSO

}

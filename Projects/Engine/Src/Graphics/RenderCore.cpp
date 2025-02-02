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

    m_sCommonRootSignature.Reset(4, 3);
    m_sCommonRootSignature[0].InitAsConstants(0, 4);
    m_sCommonRootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10);
    m_sCommonRootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 10);
    m_sCommonRootSignature[3].InitAsConstantBuffer(1);
    m_sCommonRootSignature.InitStaticSampler(0, SamplerLinearClampDesc);
    m_sCommonRootSignature.InitStaticSampler(1, SamplerPointBorderDesc);
    m_sCommonRootSignature.InitStaticSampler(2, SamplerLinearBorderDesc);
    m_sCommonRootSignature.Finalize(L"GraphicsCommonRS");

#define CreatePSO(ObjName, shaderDesc) \
    { \
        generateMips.Create(shaderDesc); \
        ObjName.SetRootSignature(m_sCommonRootSignature); \
        IDxcBlob* pShaderBlob = generateMips.GetShaderBlob(shaderDesc.typeFlags); \
        ObjName.SetComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize()); \
        ObjName.Finalize(); \
        generateMips.Release(); \
    }

    using namespace DX12;
    DXShader::Description shaderDesc;
    shaderDesc.path = "Core/GenerateMipsCS.hlsl";
    shaderDesc.typeFlags = DXShader::TypeFlag::Compute;
    DXShader generateMips;

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

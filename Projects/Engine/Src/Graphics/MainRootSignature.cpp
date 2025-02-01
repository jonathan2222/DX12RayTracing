#include "PreCompiled.h"
#include "MainRootSignature.h"

std::shared_ptr<RS::MainRootSignature> RS::MainRootSignature::Get()
{
    static std::shared_ptr<MainRootSignature> pSelf = std::make_shared<MainRootSignature>();
    return pSelf;
}

void RS::MainRootSignature::Init()
{
    m_pRootSignature = std::make_shared<RS::RootSignature>(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    auto& rootSignature = *m_pRootSignature.get();

    // TODO: Figure out the number of descriptors instead of having it hardcoded to 5!
    rootSignature[(uint)RootParams::LocalConstants][0].CBV(5, 0, 0);
    rootSignature[(uint)RootParams::LocalRBuffers][0].SRV(5, 0, 0);
    rootSignature[(uint)RootParams::LocalRTextures][0].SRV(5, 5, 0);
    rootSignature[(uint)RootParams::LocalRWBuffers][0].UAV(5, 0, 0);
    rootSignature[(uint)RootParams::LocalRWTexture][0].UAV(5, 5, 0);

    {
        CD3DX12_STATIC_SAMPLER_DESC samplerDesc{};
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        samplerDesc.Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR; // Linear sample for min, max, mag, and mip.
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        samplerDesc.MaxAnisotropy = 16;
        samplerDesc.RegisterSpace = 0;
        samplerDesc.ShaderRegister = 0;
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        samplerDesc.MipLODBias = 0;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        rootSignature.AddStaticSampler(samplerDesc);

        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        samplerDesc.Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT; // Linear sample for min, max, mag, and mip.
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        samplerDesc.MaxAnisotropy = 16;
        samplerDesc.RegisterSpace = 0;
        samplerDesc.ShaderRegister = 1;
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        samplerDesc.MipLODBias = 0;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        rootSignature.AddStaticSampler(samplerDesc);
    }
    rootSignature.Bake("Main_RootSignature");
}

void RS::MainRootSignature::Destroy()
{
    m_pRootSignature.reset();
}

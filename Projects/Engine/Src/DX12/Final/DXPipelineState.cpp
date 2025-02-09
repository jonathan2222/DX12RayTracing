//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard
//
// Modified by: Jonathan Åleskog

#include "PreCompiled.h"
#include "DXPipelineState.h"

#include "DX12/Final/DXCore.h"
#include "Utils/Misc/HashUtils.h"

#include <map>
#include <thread>
#include <mutex>

static std::map< size_t, ComPtr<ID3D12PipelineState> > s_GraphicsPSOHashMap;
static std::map< size_t, ComPtr<ID3D12PipelineState> > s_ComputePSOHashMap;

void RS::DX12::DXPSO::DestroyAll(void)
{
    s_GraphicsPSOHashMap.clear();
    s_ComputePSOHashMap.clear();
}

void RS::DX12::DXPSO::SetShader(DXShader& shader)
{
    uint32 shaderIndex = UINT32_MAX;
    DXShader::TypeFlags shaderTypes = shader.GetTypes();
    while (Utils::ForwardBitScan(shaderTypes, shaderIndex))
    {
        Utils::ClearBit(shaderTypes.Get(), shaderIndex);
        DXShader::TypeFlags shaderType = 1 << shaderIndex;
        SetShaderType(shaderType, shader);
    }
}

void RS::DX12::DXPSO::SetShaderTypes(DXShader::TypeFlag types, DXShader& shader)
{
    uint32 shaderIndex = UINT32_MAX;
    DXShader::TypeFlags shaderTypes = types;
    while (Utils::ForwardBitScan(shaderTypes, shaderIndex))
    {
        Utils::ClearBit(shaderTypes.Get(), shaderIndex);
        DXShader::TypeFlags shaderType = 1 << shaderIndex;
        SetShaderType(shaderType, shader);
    }
}

void RS::DX12::DXPSO::SetShaderDescription(const DXShader::Description& desc, const DXShader::TypeFlags& type)
{
    RS_ASSERT(Utils::IsPowerOfTwo(type), "Cannot pass in more shader types, only one is allowed!");
    std::string key = DXShader::GetDiskPathFromVirtualPath(desc.path, desc.isInternalPath);
    auto it = m_ShaderDescs.find(key);
    if (it != m_ShaderDescs.end())
        it->second.types |= type.GetConst();
    else
        m_ShaderDescs[key] = ShaderBundle{.description = desc, .types = type.GetConst()};

    const uint32 index = Utils::IndexOfPow2Bit(type.GetConst());
    m_HashCodeShaders[index] = Utils::Hash64(desc.path);
    m_HashCodeShaders[index] = Utils::Hash64(desc.isInternalPath, m_HashCodeShaders[index]);

    m_ShaderFlags |= type;
}

RS::DX12::DXGraphicsPSO::DXGraphicsPSO(const wchar_t* Name)
    : DXPSO(Name)
{
    ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
    m_PSODesc.NodeMask = 1;
    m_PSODesc.SampleMask = 0xFFFFFFFFu;
    m_PSODesc.SampleDesc.Count = 1;
    m_PSODesc.InputLayout.NumElements = 0;
}

void RS::DX12::DXGraphicsPSO::SetBlendState(const D3D12_BLEND_DESC& BlendDesc)
{
    m_PSODesc.BlendState = BlendDesc;
    m_HashCodeBlendState = Utils::Hash64State(&BlendDesc);
}

void RS::DX12::DXGraphicsPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc)
{
    m_PSODesc.RasterizerState = RasterizerDesc;
    m_HashCodeRasterizerState = Utils::Hash64State(&RasterizerDesc);
}

void RS::DX12::DXGraphicsPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc)
{
    m_PSODesc.DepthStencilState = DepthStencilDesc;
    m_HashCodeDepthStencilState = Utils::Hash64State(&DepthStencilDesc);
}

void RS::DX12::DXGraphicsPSO::SetSampleMask(UINT SampleMask)
{
    m_PSODesc.SampleMask = SampleMask;
    m_HashCodeSampleMask = Utils::Hash64(SampleMask);
}

void RS::DX12::DXGraphicsPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
{
    RS_ASSERT(TopologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, "Can't draw with undefined topology");
    m_PSODesc.PrimitiveTopologyType = TopologyType;
    m_HashCodePrimitiveTopologyType = Utils::Hash64State(&TopologyType);
}

void RS::DX12::DXGraphicsPSO::SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
    SetRenderTargetFormats(0, nullptr, DSVFormat, MsaaCount, MsaaQuality);
}

void RS::DX12::DXGraphicsPSO::SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
    SetRenderTargetFormats(1, &RTVFormat, DSVFormat, MsaaCount, MsaaQuality);
}

void RS::DX12::DXGraphicsPSO::SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
    RS_ASSERT(NumRTVs == 0 || RTVFormats != nullptr, "Null format array conflicts with non-zero length");
    for (UINT i = 0; i < NumRTVs; ++i)
    {
        RS_ASSERT(RTVFormats[i] != DXGI_FORMAT_UNKNOWN);
        m_PSODesc.RTVFormats[i] = RTVFormats[i];
    }
    for (UINT i = NumRTVs; i < m_PSODesc.NumRenderTargets; ++i)
        m_PSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
    m_PSODesc.NumRenderTargets = NumRTVs;
    m_PSODesc.DSVFormat = DSVFormat;
    m_PSODesc.SampleDesc.Count = MsaaCount;
    m_PSODesc.SampleDesc.Quality = MsaaQuality;

    m_HashCodeRenderTargetFormats = Utils::Hash64State(&m_PSODesc.RTVFormats[0], 8);
    m_HashCodeRenderTargetFormats = Utils::Hash64(m_PSODesc.NumRenderTargets, m_HashCodeRenderTargetFormats);
    m_HashCodeRenderTargetFormats = Utils::Hash64(m_PSODesc.DSVFormat, m_HashCodeRenderTargetFormats);
    m_HashCodeRenderTargetFormats = Utils::Hash64(m_PSODesc.SampleDesc.Count, m_HashCodeRenderTargetFormats);
    m_HashCodeRenderTargetFormats = Utils::Hash64(m_PSODesc.SampleDesc.Quality, m_HashCodeRenderTargetFormats);
}

void RS::DX12::DXGraphicsPSO::SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs)
{
    m_PSODesc.InputLayout.NumElements = NumElements;

    if (NumElements > 0)
    {
        D3D12_INPUT_ELEMENT_DESC* NewElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * NumElements);
        memcpy(NewElements, pInputElementDescs, NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
        m_InputLayouts.reset((const D3D12_INPUT_ELEMENT_DESC*)NewElements);
    }
    else
        m_InputLayouts = nullptr;

    m_HashCodeInputLayout = Utils::Hash64State(pInputElementDescs, NumElements);
}

void RS::DX12::DXGraphicsPSO::SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps)
{
    m_PSODesc.IBStripCutValue = IBProps;
    m_HashCodePrimitiveRestart = Utils::Hash64State(&IBProps);
}

void RS::DX12::DXGraphicsPSO::SetShaderType(DXShader::TypeFlag type, DXShader& shader)
{
    RS_ASSERT(Utils::IsPowerOfTwo(type), "Cannot pass in more shader types, only one is allowed!");
    SetShaderDescription(shader.GetDescription(), type);
    IDxcBlob* pShaderBlob = shader.GetShaderBlob(type);
    switch (type)
    {
    case DXShader::TypeFlag::Pixel: SetPixelShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize()); return;
    case DXShader::TypeFlag::Vertex: SetVertexShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize()); return;
    case DXShader::TypeFlag::Geometry: SetGeometryShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize()); return;
    case DXShader::TypeFlag::Domain: SetDomainShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize()); return;
    case DXShader::TypeFlag::Hull: SetHullShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize()); return;
    default:
        RS_ASSERT_ALWAYS("Shader of type '{}' is not supported for DXGraphicsPSO!", type.GetName());
        break;
    }
}

void RS::DX12::DXGraphicsPSO::SetVertexShader(DXShader& shader)
{
    SetShaderType(DXShader::TypeFlag::Vertex, shader);
}

void RS::DX12::DXGraphicsPSO::SetPixelShader(DXShader& shader)
{
    SetShaderType(DXShader::TypeFlag::Pixel, shader);
}

void RS::DX12::DXGraphicsPSO::SetGeometryShader(DXShader& shader)
{
    SetShaderType(DXShader::TypeFlag::Geometry, shader);
}

void RS::DX12::DXGraphicsPSO::SetHullShader(DXShader& shader)
{
    SetShaderType(DXShader::TypeFlag::Hull, shader);
}

void RS::DX12::DXGraphicsPSO::SetDomainShader(DXShader& shader)
{
    SetShaderType(DXShader::TypeFlag::Domain, shader);
}

ID3D12PipelineState* RS::DX12::DXGraphicsPSO::Finalize(bool detach)
{
    // Make sure the root signature is finalized first
    m_PSODesc.pRootSignature = m_RootSignature->GetSignature();
    RS_ASSERT(m_PSODesc.pRootSignature != nullptr);

    m_PSODesc.InputLayout.pInputElementDescs = nullptr;
    m_HashCode = Utils::Hash64State(&m_PSODesc);
    m_HashCode = Utils::Hash64State(m_InputLayouts.get(), m_PSODesc.InputLayout.NumElements, m_HashCode);
    m_PSODesc.InputLayout.pInputElementDescs = m_InputLayouts.get();

    //m_HashCode = Utils::Hash64State(&m_HashCodeShaders[0], DXShader::TypeFlag::COUNT - 1, m_HashCodeRootSignature);
    //m_HashCode = Utils::Hash64(m_HashCodeBlendState, m_HashCode);
    //m_HashCode = Utils::Hash64(m_HashCodeRasterizerState, m_HashCode);
    //m_HashCode = Utils::Hash64(m_HashCodeDepthStencilState, m_HashCode);
    //m_HashCode = Utils::Hash64(m_HashCodeDepthTargetFormat, m_HashCode);
    //m_HashCode = Utils::Hash64(m_HashCodeInputLayout, m_HashCode);
    //m_HashCode = Utils::Hash64(m_HashCodePrimitiveRestart, m_HashCode);
    //m_HashCode = Utils::Hash64(m_HashCodePrimitiveTopologyType, m_HashCode);
    //m_HashCode = Utils::Hash64(m_HashCodeRenderTargetFormats, m_HashCode);
    //m_HashCode = Utils::Hash64(m_HashCodeSampleMask, m_HashCode);

    ID3D12PipelineState** PSORef = nullptr;
    bool firstCompile = false;
    {
        static std::mutex s_HashMapMutex;
        std::lock_guard<std::mutex> CS(s_HashMapMutex);
        auto iter = s_GraphicsPSOHashMap.find(m_HashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == s_GraphicsPSOHashMap.end())
        {
            firstCompile = true;
            PSORef = s_GraphicsPSOHashMap[m_HashCode].GetAddressOf();
        }
        else
            PSORef = iter->second.GetAddressOf();
    }

    ID3D12PipelineState* pPSO = nullptr;

    if (firstCompile)
    {
        RS_ASSERT(m_PSODesc.DepthStencilState.DepthEnable != (m_PSODesc.DSVFormat == DXGI_FORMAT_UNKNOWN));
        DXCall(DXCore::GetDevice()->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&pPSO)));
        s_GraphicsPSOHashMap[m_HashCode].Attach(pPSO);
        pPSO->SetName(m_Name);
    }
    else
    {
        while (*PSORef == nullptr)
            std::this_thread::yield();
        pPSO = *PSORef;
    }

    if (!detach)
        m_PSO = pPSO;

    return pPSO;
}

RS::DX12::DXComputePSO::DXComputePSO(const wchar_t* Name)
    : DXPSO(Name)
{
    ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
    m_PSODesc.NodeMask = 1;
}

void RS::DX12::DXComputePSO::SetComputeShader(DXShader& shader)
{
    SetShaderType(DXShader::TypeFlag::Compute, shader);
}

ID3D12PipelineState* RS::DX12::DXComputePSO::Finalize(bool detach)
{
    // Make sure the root signature is finalized first
    m_PSODesc.pRootSignature = m_RootSignature->GetSignature();
    RS_ASSERT(m_PSODesc.pRootSignature != nullptr);

    m_HashCode = Utils::Hash64State(&m_PSODesc);
    //m_HashCode = Utils::Hash64State(&m_HashCodeShaders[0], DXShader::TypeFlag::COUNT - 1, m_HashCodeRootSignature);

    ID3D12PipelineState** PSORef = nullptr;
    bool firstCompile = false;
    {
        static std::mutex s_HashMapMutex;
        std::lock_guard<std::mutex> CS(s_HashMapMutex);
        auto iter = s_ComputePSOHashMap.find(m_HashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == s_ComputePSOHashMap.end())
        {
            firstCompile = true;
            PSORef = s_ComputePSOHashMap[m_HashCode].GetAddressOf();
        }
        else
            PSORef = iter->second.GetAddressOf();
    }

    ID3D12PipelineState* pPSO = nullptr;
    if (firstCompile)
    {
        DXCall(DXCore::GetDevice()->CreateComputePipelineState(&m_PSODesc, IID_PPV_ARGS(&pPSO)));
        s_ComputePSOHashMap[m_HashCode].Attach(pPSO);
        pPSO->SetName(m_Name);
    }
    else
    {
        while (*PSORef == nullptr)
            std::this_thread::yield();
        pPSO = *PSORef;
    }

    if (!detach)
        m_PSO = pPSO;

    return pPSO;
}

void RS::DX12::DXComputePSO::SetShaderType(DXShader::TypeFlag type, DXShader& shader)
{
    RS_ASSERT(Utils::IsPowerOfTwo(type), "Cannot pass in more shader types, only one is allowed!");
    SetShaderDescription(shader.GetDescription(), type);
    IDxcBlob* pShaderBlob = shader.GetShaderBlob(type);
    switch (type)
    {
    case DXShader::TypeFlag::Compute: SetComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize()); return;
    default:
        RS_ASSERT_ALWAYS("Shader of type '{}' is not supported for DXComputePSO!", type.GetName());
        break;
    }
}

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

void RS::DX12::DXPSO::SetShaderDescription(const DXShader::Description& desc, const DXShader::TypeFlags& type)
{
    std::string key = DXShader::GetDiskPathFromVirtualPath(desc.path, desc.isInternalPath);
    auto it = m_ShaderDescs.find(key);
    if (it != m_ShaderDescs.end())
        it->second.types |= type.GetConst();
    else
        m_ShaderDescs[key] = ShaderBundle{.description = desc, .types = type.GetConst()};
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
}

void RS::DX12::DXGraphicsPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc)
{
    m_PSODesc.RasterizerState = RasterizerDesc;
}

void RS::DX12::DXGraphicsPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc)
{
    m_PSODesc.DepthStencilState = DepthStencilDesc;
}

void RS::DX12::DXGraphicsPSO::SetSampleMask(UINT SampleMask)
{
    m_PSODesc.SampleMask = SampleMask;
}

void RS::DX12::DXGraphicsPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
{
    RS_ASSERT(TopologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, "Can't draw with undefined topology");
    m_PSODesc.PrimitiveTopologyType = TopologyType;
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
}

void RS::DX12::DXGraphicsPSO::SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps)
{
    m_PSODesc.IBStripCutValue = IBProps;
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

void RS::DX12::DXGraphicsPSO::Finalize()
{
    // Make sure the root signature is finalized first
    m_PSODesc.pRootSignature = m_RootSignature->GetSignature();
    RS_ASSERT(m_PSODesc.pRootSignature != nullptr);

    m_PSODesc.InputLayout.pInputElementDescs = nullptr;
    size_t HashCode = Utils::Hash64State(&m_PSODesc);
    HashCode = Utils::Hash64State(m_InputLayouts.get(), m_PSODesc.InputLayout.NumElements, HashCode);
    m_PSODesc.InputLayout.pInputElementDescs = m_InputLayouts.get();

    ID3D12PipelineState** PSORef = nullptr;
    bool firstCompile = false;
    {
        static std::mutex s_HashMapMutex;
        std::lock_guard<std::mutex> CS(s_HashMapMutex);
        auto iter = s_GraphicsPSOHashMap.find(HashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == s_GraphicsPSOHashMap.end())
        {
            firstCompile = true;
            PSORef = s_GraphicsPSOHashMap[HashCode].GetAddressOf();
        }
        else
            PSORef = iter->second.GetAddressOf();
    }

    if (firstCompile)
    {
        m_PSO = nullptr; // Will be deleted when calling DeleteAll()
        RS_ASSERT(m_PSODesc.DepthStencilState.DepthEnable != (m_PSODesc.DSVFormat == DXGI_FORMAT_UNKNOWN));
        DXCall(DXCore::GetDevice()->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO)));
        s_GraphicsPSOHashMap[HashCode].Attach(m_PSO);
        m_PSO->SetName(m_Name);
    }
    else
    {
        while (*PSORef == nullptr)
            std::this_thread::yield();
        m_PSO = *PSORef;
    }
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

void RS::DX12::DXComputePSO::Finalize()
{
    // Make sure the root signature is finalized first
    m_PSODesc.pRootSignature = m_RootSignature->GetSignature();
    RS_ASSERT(m_PSODesc.pRootSignature != nullptr);

    size_t HashCode = Utils::Hash64State(&m_PSODesc);

    ID3D12PipelineState** PSORef = nullptr;
    bool firstCompile = false;
    {
        static std::mutex s_HashMapMutex;
        std::lock_guard<std::mutex> CS(s_HashMapMutex);
        auto iter = s_ComputePSOHashMap.find(HashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == s_ComputePSOHashMap.end())
        {
            firstCompile = true;
            PSORef = s_ComputePSOHashMap[HashCode].GetAddressOf();
        }
        else
            PSORef = iter->second.GetAddressOf();
    }

    if (firstCompile)
    {
        m_PSO = nullptr; // Will be deleted when calling DeleteAll()
        DXCall(DXCore::GetDevice()->CreateComputePipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO)));
        s_ComputePSOHashMap[HashCode].Attach(m_PSO);
        m_PSO->SetName(m_Name);
    }
    else
    {
        while (*PSORef == nullptr)
            std::this_thread::yield();
        m_PSO = *PSORef;
    }
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

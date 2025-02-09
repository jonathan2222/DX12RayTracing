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

#pragma once

#include "DX12/Final/DX12Defines.h"
#include "DX12/Final/DXRootSignature.h"
#include "DX12/Final/DXShader.h"

namespace RS::DX12
{
    class DXPSO
    {
    public:
        friend class DXCommandContext;
    public:

        DXPSO(const wchar_t* Name) : m_Name(Name), m_RootSignature(nullptr), m_PSO(nullptr)
        {
            for (uint64& hash : m_HashCodeShaders)
                hash = 0u;

            static uint64 s_Generator = 0u;
            m_ID = ++s_Generator;
        }

        static void DestroyAll(void);

        void SetRootSignature(const DXRootSignature& BindMappings)
        {
            m_RootSignature = &BindMappings;
            m_HashCodeRootSignature = Utils::Hash64(m_RootSignature);
        }

        const DXRootSignature& GetRootSignature(void) const
        {
            RS_ASSERT(m_RootSignature != nullptr);
            return *m_RootSignature;
        }

        ID3D12PipelineState* GetPipelineStateObject(void) const { return m_PSO; }

        void SetShader(DXShader& shader);
        void SetShaderTypes(DXShader::TypeFlag types, DXShader& shader);
        virtual void SetShaderType(DXShader::TypeFlag type, DXShader& shader) = 0;
        virtual ID3D12PipelineState* Finalize(bool detach = false) = 0;

        uint64 GetID() const { return m_ID; }
        DXShader::TypeFlag GetShaderFlags() const { return m_ShaderFlags; }

    protected:
        void SetShaderDescription(const DXShader::Description& desc, const DXShader::TypeFlags& type);

        void SetPipelineStateObject(ID3D12PipelineState* pPSO) { m_PSO = pPSO; }

    protected:
        const wchar_t* m_Name;

        const DXRootSignature* m_RootSignature;

        // Hash code for each settable state.
        uint64 m_HashCodeRootSignature = 0u;
        uint64 m_HashCodeShaders[DXShader::TypeFlags::COUNT-1];
        // Main hash code
        uint64 m_HashCode = 0u;
        ID3D12PipelineState* m_PSO;
        uint64 m_ID = 0u;
        DXShader::TypeFlag m_ShaderFlags = DXShader::TypeFlag::NONE;

        struct ShaderBundle
        {
            DXShader::Description description;
            uint32 types;
        };
        std::unordered_map<std::string, ShaderBundle> m_ShaderDescs;
    };

    class DXGraphicsPSO : public DXPSO
    {
        friend class CommandContext;

    public:

        // Start with empty state
        DXGraphicsPSO(const wchar_t* Name = L"Unnamed Graphics PSO");

        void SetBlendState(const D3D12_BLEND_DESC& BlendDesc);
        void SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc);
        void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc);
        void SetSampleMask(UINT SampleMask);
        void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType);
        void SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
        void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
        void SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
        void SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs);
        void SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps);

        void SetVertexShader(DXShader& shader);
        void SetPixelShader(DXShader& shader);
        void SetGeometryShader(DXShader& shader);
        void SetHullShader(DXShader& shader);
        void SetDomainShader(DXShader& shader);
        void SetShaderType(DXShader::TypeFlag type, DXShader& shader) override;

        // Perform validation and compute a hash value for fast state block comparisons
        ID3D12PipelineState* Finalize(bool detach = false) override;

    private:
        // These const_casts shouldn't be necessary, but we need to fix the API to accept "const void* pShaderBytecode"
        void SetVertexShader(const void* Binary, size_t Size) { m_PSODesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
        void SetPixelShader(const void* Binary, size_t Size) { m_PSODesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
        void SetGeometryShader(const void* Binary, size_t Size) { m_PSODesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
        void SetHullShader(const void* Binary, size_t Size) { m_PSODesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
        void SetDomainShader(const void* Binary, size_t Size) { m_PSODesc.DS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }

    private:

        D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc;
        std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC> m_InputLayouts;

        // Hashes
        uint64 m_HashCodeBlendState = 0u;
        uint64 m_HashCodeRasterizerState = 0u;
        uint64 m_HashCodeDepthStencilState = 0u;
        uint64 m_HashCodeSampleMask = 0u;
        uint64 m_HashCodePrimitiveTopologyType = 0u;
        uint64 m_HashCodeDepthTargetFormat = 0u;
        uint64 m_HashCodeRenderTargetFormats = 0u;
        uint64 m_HashCodeInputLayout = 0u;
        uint64 m_HashCodePrimitiveRestart = 0u;
    };


    class DXComputePSO : public DXPSO
    {
        friend class CommandContext;

    public:
        DXComputePSO(const wchar_t* Name = L"Unnamed Compute PSO");

        void SetComputeShader(DXShader& shader);
        void SetShaderType(DXShader::TypeFlag type, DXShader& shader) override;

        ID3D12PipelineState* Finalize(bool detatch = false) override;

    private:
        void SetComputeShader(const void* Binary, size_t Size) { m_PSODesc.CS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }

    private:
        D3D12_COMPUTE_PIPELINE_STATE_DESC m_PSODesc;
    };
}
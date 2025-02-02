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

#include "DX12/Final/DXCore.h"

namespace RS::DX12
{
    class DXRootSignature;

    class DXIndirectParameter
    {
        friend class DXCommandSignature;
    public:

        DXIndirectParameter()
        {
            m_IndirectParam.Type = (D3D12_INDIRECT_ARGUMENT_TYPE)0xFFFFFFFF;
        }

        void Draw()
        {
            m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
        }

        void DrawIndexed()
        {
            m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
        }

        void Dispatch()
        {
            m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
        }

        void VertexBufferView(UINT slot)
        {
            m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
            m_IndirectParam.VertexBuffer.Slot = slot;
        }

        void IndexBufferView()
        {
            m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
        }

        void Constant(UINT rootParameterIndex, UINT destOffsetIn32BitValues, UINT num32BitValuesToSet)
        {
            m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
            m_IndirectParam.Constant.RootParameterIndex = rootParameterIndex;
            m_IndirectParam.Constant.DestOffsetIn32BitValues = destOffsetIn32BitValues;
            m_IndirectParam.Constant.Num32BitValuesToSet = num32BitValuesToSet;
        }

        void ConstantBufferView(UINT rootParameterIndex)
        {
            m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
            m_IndirectParam.ConstantBufferView.RootParameterIndex = rootParameterIndex;
        }

        void ShaderResourceView(UINT rootParameterIndex)
        {
            m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
            m_IndirectParam.ShaderResourceView.RootParameterIndex = rootParameterIndex;
        }

        void UnorderedAccessView(UINT rootParameterIndex)
        {
            m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
            m_IndirectParam.UnorderedAccessView.RootParameterIndex = rootParameterIndex;
        }

        const D3D12_INDIRECT_ARGUMENT_DESC& GetDesc(void) const { return m_IndirectParam; }

    protected:

        D3D12_INDIRECT_ARGUMENT_DESC m_IndirectParam;
    };

    class DXCommandSignature
    {
    public:
        DXCommandSignature(UINT numParams = 0) : m_Finalized(FALSE), m_NumParameters(numParams)
        {
            Reset(numParams);
        }

        void Destroy()
        {
            m_pSignature = nullptr;
            m_ParamArray = nullptr;
        }

        void Reset(UINT numParams)
        {
            if (numParams > 0)
                m_ParamArray.reset(new DXIndirectParameter[numParams]);
            else
                m_ParamArray = nullptr;

            m_NumParameters = numParams;
        }

        DXIndirectParameter& operator[] (size_t entryIndex)
        {
            RS_ASSERT(entryIndex < m_NumParameters);
            return m_ParamArray.get()[entryIndex];
        }

        const DXIndirectParameter& operator[] (size_t entryIndex) const
        {
            RS_ASSERT(entryIndex < m_NumParameters);
            return m_ParamArray.get()[entryIndex];
        }

        void Finalize(const DXRootSignature* pRootSignature = nullptr);

        ID3D12CommandSignature* GetSignature() const { return m_pSignature.Get(); }

    protected:

        BOOL m_Finalized;
        UINT m_NumParameters;
        std::unique_ptr<DXIndirectParameter[]> m_ParamArray;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_pSignature;
    };

}

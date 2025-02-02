#include "PreCompiled.h"
#include "DXCommandSignature.h"
#include "DX12/Final/DXRootSignature.h"

void RS::DX12::DXCommandSignature::Finalize(const DXRootSignature* pRootSignature)
{
    if (m_Finalized)
        return;

    UINT byteStride = 0;
    bool requiresRootSignature = false;

    for (UINT i = 0; i < m_NumParameters; ++i)
    {
        switch (m_ParamArray[i].GetDesc().Type)
        {
        case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
            byteStride += sizeof(D3D12_DRAW_ARGUMENTS);
            break;
        case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
            byteStride += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
            break;
        case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
            byteStride += sizeof(D3D12_DISPATCH_ARGUMENTS);
            break;
        case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
            byteStride += m_ParamArray[i].GetDesc().Constant.Num32BitValuesToSet * 4;
            requiresRootSignature = true;
            break;
        case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
            byteStride += sizeof(D3D12_VERTEX_BUFFER_VIEW);
            break;
        case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW:
            byteStride += sizeof(D3D12_INDEX_BUFFER_VIEW);
            break;
        case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
        case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
        case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW:
            byteStride += 8;
            requiresRootSignature = true;
            break;
        }
    }

    D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc;
    CommandSignatureDesc.ByteStride = byteStride;
    CommandSignatureDesc.NumArgumentDescs = m_NumParameters;
    CommandSignatureDesc.pArgumentDescs = (const D3D12_INDIRECT_ARGUMENT_DESC*)m_ParamArray.get();
    CommandSignatureDesc.NodeMask = 1;

    Microsoft::WRL::ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

    ID3D12RootSignature* pRootSig = pRootSignature ? pRootSignature->GetSignature() : nullptr;
    if (requiresRootSignature)
    {
        RS_ASSERT(pRootSig != nullptr);
    }
    else
    {
        pRootSig = nullptr;
    }

    DXCall(DXCore::GetDevice()->CreateCommandSignature(&CommandSignatureDesc, pRootSig, IID_PPV_ARGS(&m_pSignature)));

    m_pSignature->SetName(L"CommandSignature");

    m_Finalized = TRUE;
}

#include "PreCompiled.h"
#include "GraphicsPSO.h"

#include "DX12/NewCore/Shader.h"
#include "DX12/NewCore/RootSignature.h"

using namespace RS;

GraphicsPSO::GraphicsPSO()
{
	InitDefaults();
}

GraphicsPSO::~GraphicsPSO()
{
	if (m_pPipelineState)
	{
		m_pPipelineState->Release();
		m_pPipelineState = nullptr;
	}
}

void GraphicsPSO::SetInputLayout(D3D12_INPUT_LAYOUT_DESC inputLayoutDesc)
{
    m_PSODesc.InputLayout = inputLayoutDesc;
}

void GraphicsPSO::SetRootSignature(std::shared_ptr<RootSignature> pRootSignature)
{
    m_PSODesc.pRootSignature = pRootSignature->GetRootSignature().Get();
    m_pRootSignature = pRootSignature;
}

void RS::GraphicsPSO::SetVS(Shader* pShader, bool supressWarnings)
{
    m_PSODesc.VS = pShader->GetShaderByteCode(Shader::TypeFlag::Vertex, supressWarnings);
}

void RS::GraphicsPSO::SetPS(Shader* pShader, bool supressWarnings)
{
    m_PSODesc.PS = pShader->GetShaderByteCode(Shader::TypeFlag::Pixel, supressWarnings);
}

void RS::GraphicsPSO::SetDS(Shader* pShader, bool supressWarnings)
{
    m_PSODesc.DS = pShader->GetShaderByteCode(Shader::TypeFlag::Domain, supressWarnings);
}

void RS::GraphicsPSO::SetHS(Shader* pShader, bool supressWarnings)
{
    m_PSODesc.HS = pShader->GetShaderByteCode(Shader::TypeFlag::Hull, supressWarnings);
}

void RS::GraphicsPSO::SetGS(Shader* pShader, bool supressWarnings)
{
    m_PSODesc.GS = pShader->GetShaderByteCode(Shader::TypeFlag::Geometry, supressWarnings);
}

void RS::GraphicsPSO::SetShader(Shader* pShader)
{
    SetVS(pShader, true);
    SetPS(pShader, true);
    SetDS(pShader, true);
    SetHS(pShader, true);
    SetGS(pShader, true);
}

void RS::GraphicsPSO::SetRasterizerState(D3D12_RASTERIZER_DESC rasterizerDesc)
{
    m_PSODesc.RasterizerState = rasterizerDesc;
}

void RS::GraphicsPSO::SetBlendState(D3D12_BLEND_DESC blendDesc)
{
    m_PSODesc.BlendState = blendDesc;
}

void RS::GraphicsPSO::SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC depthStencilDesc)
{
    m_PSODesc.DepthStencilState = depthStencilDesc;
}

void RS::GraphicsPSO::SetIndexBufferStripCutValue(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE cutValue)
{
    m_PSODesc.IBStripCutValue = cutValue;
}

void RS::GraphicsPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
    m_PSODesc.PrimitiveTopologyType = topologyType;
}

void RS::GraphicsPSO::SetPSOFlags(D3D12_PIPELINE_STATE_FLAGS flags)
{
    m_PSODesc.Flags = flags;
}

void RS::GraphicsPSO::SetSampleMask(uint32 sampleMask)
{
    m_PSODesc.SampleMask = sampleMask;
}

void RS::GraphicsPSO::SetSampleDesc(DXGI_SAMPLE_DESC sampleDesc)
{
    m_PSODesc.SampleDesc = sampleDesc;
}

void RS::GraphicsPSO::SetRTVFormats(const std::vector<DXGI_FORMAT>& formats)
{
    m_PSODesc.NumRenderTargets = (UINT)formats.size();
    for (uint i = 0; i < 8; ++i)
        m_PSODesc.RTVFormats[i] = formats[i];
}

void RS::GraphicsPSO::SetDSVFormat(DXGI_FORMAT format)
{
    m_PSODesc.DSVFormat = format;
}

uint64 RS::GraphicsPSO::Create()
{
	// TODO: If one uses this function in a frame and the previous pso is still being used on the GPU then this might not work. Fix that.
	if (m_pPipelineState)
	{
		m_pPipelineState->Release();
		m_pPipelineState = nullptr;
	}

    ComputeHash();

	auto pDevice = RS::DX12Core3::Get()->GetD3D12Device();
	DXCall(pDevice->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_pPipelineState)));

    return m_Key;
}

void RS::GraphicsPSO::InitDefaults()
{
	m_PSODesc = {};
    //SetInputLayout();
    //SetRootSignature();
    //SetShader();
    //SetRTVFormats({ DXGI_FORMAT_R8G8B8A8_UNORM }); // TODO: Get this format from the render target.
    //SetDSVFormat(DXGI_FORMAT_D32_FLOAT); // TODO: Get this format from the render target.
    D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise = true;
    SetRasterizerState(rasterizerDesc);
    SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
    D3D12_DEPTH_STENCIL_DESC depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depthStencilState.DepthEnable = true;
    depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    depthStencilState.StencilEnable = false;
    SetDepthStencilState(depthStencilState);
    SetSampleMask(DefaultSampleMask());
    SetSampleDesc(DefaultSampleDesc());
    SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    SetIndexBufferStripCutValue(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED);
    SetPSOFlags(D3D12_PIPELINE_STATE_FLAG_NONE);
    //#ifdef RS_CONFIG_DEBUG
    //    m_PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
    //#endif
    
    // TODO: These does not have any hash yet. If used, add them!
    //m_PSODesc.CachedPSO
    m_PSODesc.NodeMask = 0; // Single GPU -> Set to 0.
    //m_PSODesc.StreamOutput;
}

void RS::GraphicsPSO::ComputeHash()
{
    m_HashStream.reset();
    UpdateHash(m_PSODesc.InputLayout);
    UpdateHash(m_pRootSignature);
    UpdateHash(m_PSODesc.VS);
    UpdateHash(m_PSODesc.PS);
    UpdateHash(m_PSODesc.DS);
    UpdateHash(m_PSODesc.HS);
    UpdateHash(m_PSODesc.GS);
    UpdateHash(m_PSODesc.RasterizerState);
    UpdateHash(m_PSODesc.BlendState);
    UpdateHash(m_PSODesc.DepthStencilState);
    UpdateHash(m_PSODesc.IBStripCutValue);
    UpdateHash(m_PSODesc.PrimitiveTopologyType);
    UpdateHash(m_PSODesc.Flags);
    UpdateHash(m_PSODesc.SampleMask);
    UpdateHash(m_PSODesc.SampleDesc);
    UpdateHash(m_PSODesc.RTVFormats);
    UpdateHash(m_PSODesc.DSVFormat);
    m_Key = m_HashStream.digest();
}

void RS::GraphicsPSO::UpdateHash(const D3D12_INPUT_LAYOUT_DESC& inputLayoutDesc)
{
    m_HashStream.update(inputLayoutDesc.pInputElementDescs, inputLayoutDesc.NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
}

void RS::GraphicsPSO::UpdateHash(std::shared_ptr<RootSignature> pRootSignature)
{
    pRootSignature->UpdateHash(m_HashStream);
}

void RS::GraphicsPSO::UpdateHash(const D3D12_SHADER_BYTECODE& byteCode)
{
    m_HashStream.update(byteCode.pShaderBytecode, byteCode.BytecodeLength);
}

void RS::GraphicsPSO::UpdateHash(const D3D12_RASTERIZER_DESC& rasterizerDesc)
{
    m_HashStream.update(&rasterizerDesc, sizeof(rasterizerDesc));
}

void RS::GraphicsPSO::UpdateHash(const D3D12_BLEND_DESC& blendDesc)
{
    m_HashStream.update(&blendDesc, sizeof(blendDesc));
}

void RS::GraphicsPSO::UpdateHash(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc)
{
    m_HashStream.update(&depthStencilDesc, sizeof(depthStencilDesc));
}

void RS::GraphicsPSO::UpdateHash(const D3D12_INDEX_BUFFER_STRIP_CUT_VALUE& cutValue)
{
    m_HashStream.update(&cutValue, sizeof(cutValue));
}

void RS::GraphicsPSO::UpdateHash(const D3D12_PRIMITIVE_TOPOLOGY_TYPE& topologyType)
{
    m_HashStream.update(&topologyType, sizeof(topologyType));
}

void RS::GraphicsPSO::UpdateHash(const D3D12_PIPELINE_STATE_FLAGS& flags)
{
    m_HashStream.update(&flags, sizeof(flags));
}

void RS::GraphicsPSO::UpdateHash(const uint32& sampleMask)
{
    m_HashStream.update(&sampleMask, sizeof(sampleMask));
}

void RS::GraphicsPSO::UpdateHash(const DXGI_SAMPLE_DESC& sampleDesc)
{
    m_HashStream.update(&sampleDesc, sizeof(sampleDesc));
}

void RS::GraphicsPSO::UpdateHash(DXGI_FORMAT formats[8])
{
    m_HashStream.update(formats, 8 * sizeof(DXGI_FORMAT));
}

void RS::GraphicsPSO::UpdateHash(const DXGI_FORMAT& format)
{
    m_HashStream.update(&format, sizeof(format));
}
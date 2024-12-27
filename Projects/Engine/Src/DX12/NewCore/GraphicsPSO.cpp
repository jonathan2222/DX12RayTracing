#include "PreCompiled.h"
#include "GraphicsPSO.h"

#include "DX12/NewCore/RootSignature.h"

using namespace RS;

GraphicsPSO::GraphicsPSO()
    : m_Key(0ull)
{
    for (uint i = 0; i < Shader::TypeFlag::COUNT; ++i)
        m_ShaderReflections[i] = { nullptr, "" };

    m_ShaderFileWatcher.Init("ShaderFileWatcherThread");
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

void RS::GraphicsPSO::SetDefaults()
{
    m_Key = 0ull;
    InitDefaults();
}

void GraphicsPSO::SetInputLayout(const std::vector<InputElementDesc>& inputElements)
{
    // We are doing this so we can save the pointer to the semantic name.
    m_InputElements = inputElements;

    m_InputElementDescs.resize(inputElements.size());
    for (uint i = 0; i < inputElements.size(); ++i)
    {
        InputElementDesc& ied = m_InputElements[i];
        D3D12_INPUT_ELEMENT_DESC& desc = m_InputElementDescs[i];
        desc.SemanticName = ied.SemanticName.c_str();
        desc.SemanticIndex = ied.SemanticIndex;
        desc.Format = ied.Format;
        desc.InputSlot = ied.InputSlot;
        desc.AlignedByteOffset = ied.AlignedByteOffset;
        desc.InputSlotClass = ied.InputSlotClass;
        desc.InstanceDataStepRate = ied.InstanceDataStepRate;
    }

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
    inputLayoutDesc.NumElements = m_InputElementDescs.size();
    inputLayoutDesc.pInputElementDescs = m_InputElementDescs.data();
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
    ShaderReflectionValidationInfo& info = m_ShaderReflections[Shader::TypeFlag::Vertex_INDEX];
    info.pReflection = pShader->GetReflection(Shader::TypeFlag::Vertex, supressWarnings);
    info.shaderPath = pShader->GetVirtualPath();
    info.shaderType = Shader::TypeFlag::Vertex_STR;
}

void RS::GraphicsPSO::SetPS(Shader* pShader, bool supressWarnings)
{
    m_PSODesc.PS = pShader->GetShaderByteCode(Shader::TypeFlag::Pixel, supressWarnings);
    ShaderReflectionValidationInfo& info = m_ShaderReflections[Shader::TypeFlag::Pixel_INDEX];
    info.pReflection = pShader->GetReflection(Shader::TypeFlag::Pixel, supressWarnings);
    info.shaderPath = pShader->GetVirtualPath();
    info.shaderType = Shader::TypeFlag::Pixel_STR;
}

void RS::GraphicsPSO::SetDS(Shader* pShader, bool supressWarnings)
{
    m_PSODesc.DS = pShader->GetShaderByteCode(Shader::TypeFlag::Domain, supressWarnings);
    ShaderReflectionValidationInfo& info = m_ShaderReflections[Shader::TypeFlag::Domain_INDEX];
    info.pReflection = pShader->GetReflection(Shader::TypeFlag::Domain, supressWarnings);
    info.shaderPath = pShader->GetVirtualPath();
    info.shaderType = Shader::TypeFlag::Domain_STR;
}

void RS::GraphicsPSO::SetHS(Shader* pShader, bool supressWarnings)
{
    m_PSODesc.HS = pShader->GetShaderByteCode(Shader::TypeFlag::Hull, supressWarnings);
    ShaderReflectionValidationInfo& info = m_ShaderReflections[Shader::TypeFlag::Hull_INDEX];
    info.pReflection = pShader->GetReflection(Shader::TypeFlag::Hull, supressWarnings);
    info.shaderPath = pShader->GetVirtualPath();
    info.shaderType = Shader::TypeFlag::Hull_STR;
}

void RS::GraphicsPSO::SetGS(Shader* pShader, bool supressWarnings)
{
    m_PSODesc.GS = pShader->GetShaderByteCode(Shader::TypeFlag::Geometry, supressWarnings);
    ShaderReflectionValidationInfo& info = m_ShaderReflections[Shader::TypeFlag::Geometry_INDEX];
    m_ShaderReflections[Shader::TypeFlag::Geometry_INDEX].pReflection = pShader->GetReflection(Shader::TypeFlag::Geometry, supressWarnings);
    m_ShaderReflections[Shader::TypeFlag::Geometry_INDEX].shaderPath = pShader->GetVirtualPath();
    m_ShaderReflections[Shader::TypeFlag::Geometry_INDEX].shaderType = Shader::TypeFlag::Geometry_STR;
}

void RS::GraphicsPSO::SetShader(Shader* pShader)
{
    m_ShaderDescription = pShader->GetDescription();
    if (!m_ShaderFileWatcher.HasExactListener(pShader->GetPath()))
    {
        m_ShaderFileWatcher.AddFileListener(pShader->GetPath(),
            [&](const std::filesystem::path& path, FileWatcher::FileStatus status)
            {
                Shader shader;
                bool updatedSuccessfully = shader.Create(m_ShaderDescription);
                if (updatedSuccessfully)
                {
                    {
                        std::lock_guard<std::mutex> lock(m_Mutex);
                        SetVS(&shader, true);
                        SetPS(&shader, true);
                        SetDS(&shader, true);
                        SetHS(&shader, true);
                        SetGS(&shader, true);
                    }
                    Create();

                    shader.Release();
                }
            });
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
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
    for (uint i = 0; i < m_PSODesc.NumRenderTargets; ++i)
        m_PSODesc.RTVFormats[i] = formats[i];
}

void RS::GraphicsPSO::SetDSVFormat(DXGI_FORMAT format)
{
    m_PSODesc.DSVFormat = format;
}

uint64 RS::GraphicsPSO::Create()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
	// TODO: If one uses this function in a frame and the previous pso is still being used on the GPU then this might not work. Fix that.
	if (m_pPipelineState)
	{
		m_pPipelineState->Release();
		m_pPipelineState = nullptr;
	}

    ComputeHash();

    ValidateInputLayoutMatchingShader();

	auto pDevice = RS::DX12Core3::Get()->GetD3D12Device();
	DXCall(pDevice->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_pPipelineState)));

    return m_Key;
}

uint64 RS::GraphicsPSO::GetKey() const
{
    RS_ASSERT(m_Key != 0, "Missing key!");
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
    m_HashStream.update(formats, m_PSODesc.NumRenderTargets * sizeof(DXGI_FORMAT));
}

void RS::GraphicsPSO::UpdateHash(const DXGI_FORMAT& format)
{
    m_HashStream.update(&format, sizeof(format));
}

void RS::GraphicsPSO::ValidateInputLayoutMatchingShader()
{
    if (m_ShaderReflections[Shader::TypeFlag::Vertex].pReflection != nullptr)
    {
        ShaderReflectionValidationInfo& reflectionValidationInfo = m_ShaderReflections[Shader::TypeFlag::Vertex];
        std::string errorStr = std::format("Input Layout validation error with shader \"{}\" [{}].",
            reflectionValidationInfo.shaderPath.c_str(),
            reflectionValidationInfo.shaderType.c_str());
        ID3D12ShaderReflection* pReflection = reflectionValidationInfo.pReflection;
        D3D12_SHADER_DESC d12ShaderDesc{};
        DXCall(pReflection->GetDesc(&d12ShaderDesc));

        //RS_ASSERT(d12ShaderDesc.InputParameters == m_PSODesc.InputLayout.NumElements,
        //    "{} Number of layout input parameters differ from what is defined in the shader!", errorStr.c_str());
        if (d12ShaderDesc.InputParameters != m_PSODesc.InputLayout.NumElements)
        {
            RS_LOG_WARNING("{} Number of layout input parameters differ from what is defined in the shader!", errorStr.c_str());
        }


        D3D12_SIGNATURE_PARAMETER_DESC inputParameter = {};
        for (uint i = 0; i < std::min(d12ShaderDesc.InputParameters, m_PSODesc.InputLayout.NumElements); ++i)
        {
            DXCall(pReflection->GetInputParameterDesc(i, &inputParameter));
            const D3D12_INPUT_ELEMENT_DESC& layoutInputParameter = m_PSODesc.InputLayout.pInputElementDescs[i];
            const char* layoutSemanticName = layoutInputParameter.SemanticName;
            const std::string_view layoutSName(layoutSemanticName);
            RS_ASSERT(!std::isdigit(layoutSName[layoutSName.size() - 1]),
                "{} Cannot have the semantic index in the layout semantic name! Name was '{}'", errorStr.c_str(), layoutSName);

            RS_ASSERT(layoutInputParameter.SemanticIndex == inputParameter.SemanticIndex,
                "{} Semantic Index differ for semantic {}! Layout has index {} while shader has index {}",
                errorStr.c_str(), layoutSName, layoutInputParameter.SemanticIndex, inputParameter.SemanticIndex);

            const std::string_view shaderSName(inputParameter.SemanticName);
            RS_ASSERT(layoutSName == shaderSName, "{} Semantic names differ! Layout has {}, while shader has {}.",
                errorStr.c_str(), layoutSemanticName, inputParameter.SemanticName);
        }
    }
}

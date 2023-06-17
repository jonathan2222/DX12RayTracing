#include "PreCompiled.h"
#include "DX12Core3.h"

#include "Core/EngineLoop.h"

// TODO: Remove/Move these!
#include "DX12/Shader.h"

RS::DX12Core3::~DX12Core3()
{
}

void RS::DX12Core3::Flush()
{
    m_pDirectCommandQueue->Flush();
}

RS::DescriptorAllocator* RS::DX12Core3::GetDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    return m_pDescriptorHeaps[type].get();
}

RS::DescriptorAllocation RS::DX12Core3::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    return GetDescriptorAllocator(type)->Allocate(1);
}

RS::DX12Core3::DX12Core3()
    : m_CurrentFrameIndex(0)
    , m_FenceValues{ 0 }
{
}

RS::DX12Core3* RS::DX12Core3::Get()
{
    static std::unique_ptr<DX12Core3> pDX12Core(new DX12Core3());
    return pDX12Core.get();
}

void RS::DX12Core3::Init(HWND window, int width, int height)
{
    const D3D_FEATURE_LEVEL d3dMinFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    const DX12::DXGIFlags dxgiFlags = DX12::DXGIFlag::REQUIRE_TEARING_SUPPORT;
    m_Device.Init(d3dMinFeatureLevel, dxgiFlags);
    m_Surface.Init(window, width, height, dxgiFlags);

    m_pDirectCommandQueue = std::make_unique<CommandQueue>(m_Device.GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_DIRECT);

    for (uint32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        m_pDescriptorHeaps[i] = std::make_unique<DescriptorAllocator>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i), 256);

    // TODO: This should be able to be outside this calss. Think what happens when we have multiple swap chains.
    m_SwapChain = std::make_unique<SwapChain>(window, width, height, dxgiFlags);

    // TODO: Remove/Move these!
    {
        CreatePipelineState();

        struct Vertex
        {
            float position[3];
            float color[4];
            float uv[2];
        };

        float scale = 0.25f;
        float aspectRatio = (float)Display::Get()->GetWidth() / Display::Get()->GetHeight();
        Vertex triangleVertices[] =
        {
            { { -scale, +scale * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } }, // TL
            { { +scale, +scale * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }, // TR
            { { -scale, -scale * aspectRatio, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } }, // BL

            { { +scale, +scale * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }, // TR
            { { +scale, -scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } }, // BR
            { { -scale, -scale * aspectRatio, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } }  // BL
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        //struct ConstantBufferData
        //{
        //    float miscData[4] = { 1.0, 1.0, 1.0, 1.0 };
        //} textIndex;

        auto pCommandList = m_pDirectCommandQueue->GetCommandList();
        m_pVertexBufferResource = pCommandList->CreateVertexBufferResource(vertexBufferSize, sizeof(Vertex), "Vertex Buffer");
        pCommandList->UploadToBuffer(m_pVertexBufferResource, vertexBufferSize, (void*)&triangleVertices[0]);
        //m_ConstantBufferResource = pCommandList->CopyBuffer(sizeof(textIndex), (void*)&textIndex, D3D12_RESOURCE_FLAG_NONE);
    }
}

void RS::DX12Core3::Release()
{
    m_Surface.Release();
    m_Device.Release();
}

void RS::DX12Core3::FreeResource(Microsoft::WRL::ComPtr<ID3D12Resource> pResource)
{
    uint32 frameIndex = GetCurrentFrameIndex();
    std::lock_guard<std::mutex> lock(m_ResourceRemovalMutex);
    m_PendingResourceRemovals[frameIndex].push_back(pResource);
}

void RS::DX12Core3::Render()
{
    const uint32 frameIndex = GetCurrentFrameIndex();
    ReleasePendingResourceRemovals(frameIndex);

    auto pCommandList = m_pDirectCommandQueue->GetCommandList();
    // ....
    pCommandList->SetRootSignature(m_pRootSignature);
    pCommandList->SetPipelineState(m_pPipelineState);
    D3D12_VIEWPORT viewport{};
    viewport.MaxDepth = 0;
    viewport.MinDepth = 1;
    viewport.TopLeftX = viewport.TopLeftY = 0;
    viewport.Width = Display::Get()->GetWidth();
    viewport.Height = Display::Get()->GetHeight();
    pCommandList->SetViewport(viewport);
    D3D12_RECT scissorRect{};
    scissorRect.left = scissorRect.top = 0;
    scissorRect.right = viewport.Width;
    scissorRect.bottom = viewport.Height;
    pCommandList->SetScissorRects(scissorRect);

    // TODO: Set rtv
    // TODO: Clear rtv (texture)
    //pCommandList->ClearTexture()

    auto pBackBuffer = m_SwapChain->GetCurrentBackBuffer();
    //pCommandList->SetRenderTarget();

    pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->SetVertexBuffers(0, m_pVertexBufferResource);

    struct RootConstData
    {
        const float tint[4] = { 1.0, 1.0, 1.0, 1.0 };
        uint32 texIndex[4] = { (uint32)-1, (uint32)-1, (uint32)-1, (uint32)-1 };
    } rootConstData;
    //rootConstData.texIndex[0] = m_Texture.GetDescriptorIndex();
    //rootConstData.texIndex[1] = rootConstData.texIndex[2] = rootConstData.texIndex[3] = m_NullTexture.GetDescriptorIndex();
    pCommandList->SetGraphicsRoot32BitConstants(0, 4 * 2, &rootConstData);

    // TODO: Bind textures. The DynamicDescriptorHeap does not support bindless I think...

    struct ConstantBufferData
    {
        float miscData[4] = { 1.0, 1.0, 1.0, 1.0 };
    } textIndex;
    pCommandList->SetGraphicsDynamicConstantBuffer(1, sizeof(textIndex), (void*)&textIndex);

    pCommandList->DrawInstanced(6, 1, 0, 0);

    m_CurrentFrameIndex = m_SwapChain->Present(nullptr);

    //m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % FRAME_BUFFER_COUNT;
}

void RS::DX12Core3::ReleaseStaleDescriptors()
{
    const uint64 frameNumber = EngineLoop::GetCurrentFrameNumber();

    for (uint32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        m_pDescriptorHeaps[i]->ReleaseStaleDescriptors(frameNumber);
}

uint32 RS::DX12Core3::GetCurrentFrameIndex() const
{
    return m_CurrentFrameIndex;
}

void RS::DX12Core3::ReleasePendingResourceRemovals(uint32 frameIndex)
{
    m_ResourceRemovalMutex.lock();
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> pendingRemovals = m_PendingResourceRemovals[frameIndex];
    m_PendingResourceRemovals[frameIndex].clear();
    m_ResourceRemovalMutex.unlock();

    // Remove resources.
    for (auto resource : pendingRemovals)
    {
        ResourceStateTracker::RemoveGlobalResourceState(resource.Get());
        resource->Release();
    }
}

// TODO: Remove/Move these!
void RS::DX12Core3::CreatePipelineState()
{
    DX12::Shader shader;
    DX12::Shader::Description shaderDesc{};
    shaderDesc.path = "TmpCore3Shaders.hlsl";
    shaderDesc.typeFlags = DX12::Shader::TypeFlag::Pixel | DX12::Shader::TypeFlag::Vertex;
    shader.Create(shaderDesc);

    // TODO: Remove this
    { // Test reflection
        ID3D12ShaderReflection* reflection = shader.GetReflection(DX12::Shader::TypeFlag::Pixel);

        D3D12_SHADER_DESC d12ShaderDesc{};
        DXCall(reflection->GetDesc(&d12ShaderDesc));

        D3D12_SHADER_INPUT_BIND_DESC desc{};
        DXCall(reflection->GetResourceBindingDesc(0, &desc));

        D3D12_SHADER_INPUT_BIND_DESC desc2{};
        DXCall(reflection->GetResourceBindingDesc(1, &desc2));
    }

    CreateRootSignature();

    auto pDevice = GetD3D12Device();

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
    // TODO: Move this, and don't hardcode it!
    {
        D3D12_INPUT_ELEMENT_DESC inputElementDesc = {};
        inputElementDesc.SemanticName = "POSITION";
        inputElementDesc.SemanticIndex = 0;
        inputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        inputElementDesc.InputSlot = 0;
        inputElementDesc.AlignedByteOffset = 0;
        inputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        inputElementDesc.InstanceDataStepRate = 0;
        inputElementDescs.push_back(inputElementDesc);

        inputElementDescs.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        inputElementDescs.push_back({ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    }

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
    inputLayoutDesc.NumElements = inputElementDescs.size();
    inputLayoutDesc.pInputElementDescs = inputElementDescs.data();

    auto GetShaderBytecode = [&](IDxcBlob* shaderObject)->CD3DX12_SHADER_BYTECODE {
        if (shaderObject)
            return CD3DX12_SHADER_BYTECODE(shaderObject->GetBufferPointer(), shaderObject->GetBufferSize());
        return CD3DX12_SHADER_BYTECODE(nullptr, 0);
    };

    // TODO: Change the pipelinestate to use subobject like below (this saves memory by only passing subobjects that we need):
    //struct
    //{
    //	struct alignas(void*)
    //	{
    //		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type{ D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE };
    //		ID3D12RootSignature* rootSigBlob;
    //	} rootSig;
    //} streams;
    //streams.rootSig.rootSigBlob = m_RootSignature;
    //
    //D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{};
    //streamDesc.pPipelineStateSubobjectStream = &streams;
    //streamDesc.SizeInBytes = sizeof(streams);
    //DXCall(pDevice->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_PipelineState)));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = inputLayoutDesc;
    psoDesc.pRootSignature = m_pRootSignature->GetRootSignature().Get();
    psoDesc.VS = GetShaderBytecode(shader.GetShaderBlob(DX12::Shader::TypeFlag::Vertex, true));
    psoDesc.PS = GetShaderBytecode(shader.GetShaderBlob(DX12::Shader::TypeFlag::Pixel, true));
    psoDesc.DS = GetShaderBytecode(shader.GetShaderBlob(DX12::Shader::TypeFlag::Domain, true));
    psoDesc.HS = GetShaderBytecode(shader.GetShaderBlob(DX12::Shader::TypeFlag::Hull, true));
    psoDesc.GS = GetShaderBytecode(shader.GetShaderBlob(DX12::Shader::TypeFlag::Geometry, true));
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    //psoDesc.DSVFormat
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    //psoDesc.CachedPSO
    psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    psoDesc.NodeMask = 0; // Single GPU -> Set to 0.
    //psoDesc.StreamOutput;
#ifdef RS_CONFIG_DEBUG
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
#else
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
#endif
    DXCall(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState)));

    shader.Release();
}

void RS::DX12Core3::CreateRootSignature()
{
    m_pRootSignature = std::make_shared<RootSignature>(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    uint32 registerSpace = 0;
    uint32 currentShaderRegisterCBV = 0;
    uint32 currentShaderRegisterSRV = 0;
    uint32 currentShaderRegisterUAV = 0;
    uint32 currentShaderRegisterSampler = 0;

    const uint32 cbvRegSpace = 1;
    const uint32 srvRegSpace = 2;
    const uint32 uavRegSpace = 3;

    auto& rootSignature = *m_pRootSignature.get();
    rootSignature[0].Constants(4 * 2, currentShaderRegisterCBV++, registerSpace);
    rootSignature[1].CBV(currentShaderRegisterCBV++, registerSpace);

    // All bindless buffers, textures overlap using different spaces.
    rootSignature[2][0].CBVBindless(0, cbvRegSpace, 0);
    rootSignature[2][1].SRVBindless(0, srvRegSpace, 0);
    rootSignature[2][2].UAVBindless(0, uavRegSpace, 0);

    {
        CD3DX12_STATIC_SAMPLER_DESC samplerDesc{};
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        samplerDesc.Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT; // Point sample for min, max, mag, mip.
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        samplerDesc.MaxAnisotropy = 16;
        samplerDesc.RegisterSpace = registerSpace;
        samplerDesc.ShaderRegister = currentShaderRegisterSampler++;
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        samplerDesc.MipLODBias = 0;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        rootSignature.AddStaticSampler(samplerDesc);
    }

    rootSignature.Bake();
}

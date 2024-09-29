#include "SandboxApp.h"

#include "Core/CorePlatform.h"
#include "DX12/NewCore/Shader.h"

#include "Maths/RSMatrix.h"

#include "Loaders/openfbx/FBXLoader.h"

//#define GLM_FORCE_LEFT_HANDED 
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"

SandboxApp::SandboxApp()
{
    Init();
}

SandboxApp::~SandboxApp()
{
    if (m_pPipelineState)
    {
        m_pPipelineState->Release();
        m_pPipelineState = nullptr;
    }
}

void SandboxApp::FixedTick()
{
}

void SandboxApp::Tick(const RS::FrameStats& frameStats)
{
    {
        auto pCommandQueue = RS::DX12Core3::Get()->GetDirectCommandQueue();
        auto pCommandList = pCommandQueue->GetCommandList();

        pCommandList->SetRootSignature(m_pRootSignature);
        pCommandList->SetPipelineState(m_pPipelineState);

        D3D12_VIEWPORT viewport{};
        viewport.MaxDepth = 0;
        viewport.MinDepth = 1;
        viewport.TopLeftX = viewport.TopLeftY = 0;
        viewport.Width = RS::Display::Get()->GetWidth();
        viewport.Height = RS::Display::Get()->GetHeight();
        pCommandList->SetViewport(viewport);

        D3D12_RECT scissorRect{};
        scissorRect.left = scissorRect.top = 0;
        scissorRect.right = viewport.Width;
        scissorRect.bottom = viewport.Height;
        pCommandList->SetScissorRect(scissorRect);

        pCommandList->SetRenderTarget(m_RenderTarget);
        auto pRenderTargetTexture = m_RenderTarget->GetAttachment(RS::AttachmentPoint::Color0);
        pCommandList->ClearTexture(pRenderTargetTexture, pRenderTargetTexture->GetClearValue()->Color);

        auto pRenderTargetDepthTexture = m_RenderTarget->GetAttachment(RS::AttachmentPoint::DepthStencil);
        pCommandList->ClearDSV(pRenderTargetDepthTexture, D3D12_CLEAR_FLAG_DEPTH,  pRenderTargetDepthTexture->GetClearValue()->DepthStencil.Depth, pRenderTargetDepthTexture->GetClearValue()->DepthStencil.Stencil);

        pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pCommandList->SetVertexBuffers(0, m_pVertexBufferResource);

        struct RootConstData
        {
            const float tint[4] = { 1.0, 1.0, 1.0, 1.0 };
            uint32 texIndex[4] = { (uint32)-1, (uint32)-1, (uint32)-1, (uint32)-1 };
        } rootConstData;
        rootConstData.texIndex[0] = 0;
        rootConstData.texIndex[1] = rootConstData.texIndex[2] = rootConstData.texIndex[3] = 1;
        pCommandList->SetGraphicsRoot32BitConstants(RootParameter::PixelData, 4 * 2, &rootConstData);

        struct ConstantBufferData
        {
            float miscData[4] = { 1.0, 1.0, 1.0, 1.0 };
        } textIndex;
        pCommandList->SetGraphicsDynamicConstantBuffer(RootParameter::PixelData2, sizeof(textIndex), (void*)&textIndex);

        struct VertexConstantBufferData
        {
            glm::mat4 transform;
            glm::mat4 camera;
        } vertexViewData;

        // Initial camera orientation and position.
        //static glm::vec3 direction(0.0f, 0.0f, 1.0f); // LH coordinate system => z is forward
        //static glm::vec3 up(0.0f, 1.0f, 0.0f);
        //static glm::vec3 position(0.0f, 0.0f, -3.0f);
        //
        //// TODO: Change to quaternions.
        //glm::vec3 right = glm::cross(direction, up);
        //up = glm::cross(right, direction);
        //
        //// -1 if a, 1 if b, else 0 (a and b is also 0).
        //auto KeysPressed = [](RS::Key a, RS::Key b)
        //{
        //    return (RS::Input::Get()->IsKeyPressed(b) ? 1.f : 0.f) - (RS::Input::Get()->IsKeyPressed(a) ? 1.f : 0.f);
        //};
        //float hAngle = frameStats.frame.currentDT * KeysPressed(RS::Key::RIGHT, RS::Key::LEFT);
        //float vAngle = frameStats.frame.currentDT * KeysPressed(RS::Key::DOWN, RS::Key::UP);
        //direction = glm::rotate(hAngle, glm::vec3{0.f, 1.0f, 0.f}) * glm::vec4(direction, 1.0f);
        //direction = glm::normalize(direction);
        //right = glm::cross(up, direction); // Ajdust right vector to match with the rotation of direction.
        //direction = glm::rotate(vAngle, right) * glm::vec4(direction, 1.0f);
        //direction = glm::normalize(direction);
        //up = glm::cross(right, direction); // Ajdust up vector to match with the rotation of direction.
        //up = glm::normalize(up);
        //
        //float speed = 10.0f;
        //float speedRight = KeysPressed(RS::Key::A, RS::Key::D) * speed;
        //float speedForward = KeysPressed(RS::Key::S, RS::Key::W) * speed;
        //float speedUp = KeysPressed(RS::Key::LEFT_SHIFT, RS::Key::SPACE) * speed;
        //position += right * frameStats.frame.currentDT * speedRight + direction * frameStats.frame.currentDT * speedForward;
        //position.y += frameStats.frame.currentDT * speedUp;
        //// TODO: Fix direction!
        ////LOG_WARNING("Dir: {}, Pos: {}", direction.ToString(), position.ToString());
        //
        //RS_New::VecNew<2u, float> newVecNoInit(RS_New::NoInit);
        //RS_New::VecNew<2u, float> newVecOnes(1.0f);
        //RS_New::VecNew<2u, float> newVecZeros;
        //RS_New::VecNew<2u, float> newVec1(1.0f, 2.0f);
        //RS_New::VecNew<2u, int32> newVec2U(-1, 2);
        //RS_New::VecNew<2u, float> newVec2F(newVec2U);
        //uint32 values[3] = { 0, 1, 2 };
        //RS_New::VecNew<3u, uint32> newVec3U(3, values);
        //
        //RS_New::VecNew<2u, float> newVec11 = RS_New::VecNew<2u, float>::Ones;
        //RS_New::VecNew<2u, float> newVec00 = RS_New::VecNew<2u, float>::Zeros;
        //
        ////RS::Mat4 view = RS::CreateCameraMat4(direction, {0.f, 1.0f, 0.0f}, position);
        ////LOG_WARNING("view: {}", view.ToString());
        ////RS::Mat4 proj = RS::CreatePerspectiveProjectionMat4(45.0f, RS::Display::Get()->GetAspectRatio(), 0.01f, 100.0f);
        ////LOG_WARNING("proj: {}", proj.ToString());
        //
        //// TODO: Try with glm and see if that fixes it!
        //glm::mat4 viewG = glm::lookAtLH((glm::vec3)direction, (glm::vec3)(direction + position), { 0.f, 1.0f, 0.0f });
        //glm::mat4 projG = glm::perspectiveLH_ZO(45.f * 3.1415f / 180.f, RS::Display::Get()->GetAspectRatio(), 0.01f, 100.0f);

        static bool sCameraIsActive = false;
        if (RS::Input::Get()->IsKeyClicked(RS::Key::C))
        {
            sCameraIsActive ^= 1;
            if (sCameraIsActive) RS::Input::Get()->LockMouse();
            else                 RS::Input::Get()->UnlockMouse();
        }
        if (sCameraIsActive)
            m_Camera.Update(frameStats.frame.currentDT);
        //static float time = 0.f;
        //time += frameStats.frame.currentDT;
        //if (time > 1.f)
        //{
        //    LOG_WARNING("Camera:");
        //    LOG_WARNING("\tPos: ({}, {}, {})", m_Camera.GetPosition().x, m_Camera.GetPosition().y, m_Camera.GetPosition().z);
        //    LOG_WARNING("\tRight: ({}, {}, {})", m_Camera.GetRight().x, m_Camera.GetRight().y, m_Camera.GetRight().z);
        //    LOG_WARNING("\tUp: ({}, {}, {})", m_Camera.GetUp().x, m_Camera.GetUp().y, m_Camera.GetUp().z);
        //    LOG_WARNING("\tForward: ({}, {}, {})", m_Camera.GetDirection().x, m_Camera.GetDirection().y, m_Camera.GetDirection().z);
        //    LOG_WARNING("----------------------------------------");
        //    time = 0.f;
        //}

        float scale = 0.5f;
        vertexViewData.camera = glm::transpose(m_Camera.GetMatrix());
        vertexViewData.transform =
        {
            scale, 0.f, 0.f, 0.f,
            0.f, scale, 0.f, 0.f,
            0.f, 0.f, scale, 0.0f,
            0.f, 0.f, 0.f, 1.f
        };
        vertexViewData.camera = vertexViewData.camera;
        vertexViewData.transform = vertexViewData.transform;
        pCommandList->SetGraphicsDynamicConstantBuffer(RootParameter::VertexData, sizeof(vertexViewData), (void*)&vertexViewData);

        pCommandList->BindTexture(RootParameter::Textures, 0, m_NormalTexture);
        pCommandList->BindTexture(RootParameter::Textures, 1, m_NullTexture);

        pCommandList->DrawInstanced(m_NumVertices, 1, 0, 0);

        // TODO: Change this to render to backbuffer instead of copy. Reason: If window gets resized this will not work.
        auto pTexture = m_RenderTarget->GetColorTextures()[0];
        pCommandList->CopyResource(RS::DX12Core3::Get()->GetSwapChain()->GetCurrentBackBuffer(), pTexture);

        pCommandQueue->ExecuteCommandList(pCommandList);
    }

    // Copy rendertarget to swapchain.
    //{
    //    auto pCommandQueue = RS::DX12Core3::Get()->GetDirectCommandQueue();
    //    auto pCommandList = pCommandQueue->GetCommandList();
    //
    //    auto pTexture = m_RenderTarget->GetColorTextures()[0];
    //    pCommandList->CopyResource(RS::DX12Core3::Get()->GetSwapChain()->GetCurrentBackBuffer(), pTexture);
    //
    //    pCommandQueue->ExecuteCommandList(pCommandList);
    //}
}

void SandboxApp::Init()
{
    CreatePipelineState();

    RS::Mesh* pMesh = RS::FBXLoader::Load("Sword2.fbx");

    const UINT vertexBufferSize = sizeof(RS::Vertex) * pMesh->vertices.size();

    auto pCommandQueue = RS::DX12Core3::Get()->GetDirectCommandQueue();
    auto pCommandList = pCommandQueue->GetCommandList();
    m_pVertexBufferResource = pCommandList->CreateVertexBufferResource(vertexBufferSize, sizeof(RS::Vertex), "Vertex Buffer");
    pCommandList->UploadToBuffer(m_pVertexBufferResource, vertexBufferSize, (void*)&pMesh->vertices[0]);
    m_NumVertices = pMesh->vertices.size();

    //m_ConstantBufferResource = pCommandList->CopyBuffer(sizeof(textIndex), (void*)&textIndex, D3D12_RESOURCE_FLAG_NONE);

    //float scale = 1.0f;
    //float aspectRatio = 1.0f;
    //RS::Vertex triangleVertices[] =
    //{
    //    { { -scale, +scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } }, // TL
    //    { { +scale, +scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }, // TR
    //    { { -scale, -scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }, // BL
    //
    //    { { +scale, +scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }, // TR
    //    { { +scale, -scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } }, // BR
    //    { { -scale, -scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }  // BL
    //};
    //m_NumVertices = 6;
    //
    //const UINT vertexBufferSize2 = sizeof(triangleVertices);
    //m_pVertexBufferResource = pCommandList->CreateVertexBufferResource(vertexBufferSize2, sizeof(RS::Vertex), "Vertex Buffer");
    //pCommandList->UploadToBuffer(m_pVertexBufferResource, vertexBufferSize2, (void*)&triangleVertices[0]);

    if (pMesh)
    {
        delete pMesh;
        pMesh = nullptr;
    }

    // Texture
    {
        std::string texturePath = RS_TEXTURE_PATH + std::string("flyToYourDream.jpg");
        std::unique_ptr<RS::CorePlatform::Image> pImage = RS::CorePlatform::Get()->LoadImageData(texturePath, RS::RS_FORMAT_R8G8B8A8_UNORM, RS::CorePlatform::ImageFlag::FLIP_Y);
        m_NormalTexture = pCommandList->CreateTexture(pImage->width, pImage->height, pImage->pData, RS::DX12::GetDXGIFormat(pImage->format), "FlyToYTourDeam Texture Resource");
    }

    // Null Texture
    {
        std::string texturePath = RS_TEXTURE_PATH + std::string("NullTexture.png");
        std::unique_ptr<RS::CorePlatform::Image> pImage = RS::CorePlatform::Get()->LoadImageData(texturePath, RS::RS_FORMAT_R8G8B8A8_UNORM, RS::CorePlatform::ImageFlag::FLIP_Y);
        m_NullTexture = pCommandList->CreateTexture(pImage->width, pImage->height, pImage->pData, RS::DX12::GetDXGIFormat(pImage->format), "Null Texture Resource");
    }

    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    clearValue.Color[0] = 0.2f;
    clearValue.Color[1] = 0.5f;
    clearValue.Color[2] = 0.1f;
    clearValue.Color[3] = 1.0f;
    m_RenderTarget = std::make_shared<RS::RenderTarget>();
    auto pRenderTexture = pCommandList->CreateTexture(
        RS::Display::Get()->GetWidth(),
        RS::Display::Get()->GetHeight(),
        nullptr,
        clearValue.Format,
        "Canvas Texture",
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &clearValue);
    m_RenderTarget->SetAttachment(RS::AttachmentPoint::Color0, pRenderTexture);

    D3D12_CLEAR_VALUE clearValueDepth;
    clearValueDepth.Format = DXGI_FORMAT_D32_FLOAT; // Only depth
    clearValueDepth.DepthStencil.Depth = 0.0f;
    clearValueDepth.DepthStencil.Stencil = 0;
    auto pDepthTexture = pCommandList->CreateTexture(
        RS::Display::Get()->GetWidth(),
        RS::Display::Get()->GetHeight(),
        nullptr,
        clearValueDepth.Format,
        "Canvas Depth Texture",
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &clearValueDepth);
    m_RenderTarget->SetAttachment(RS::AttachmentPoint::DepthStencil, pDepthTexture);

    uint64 fenceValue = pCommandQueue->ExecuteCommandList(pCommandList);

    // Wait for load to finish.
    pCommandQueue->WaitForFenceValue(fenceValue);

    float aspect = RS::Display::Get()->GetAspectRatio();
    m_Camera.Init(aspect, 45.f, { 0.f, 1.f, -2.f }, { 0.f, 1.0f, 1.0f }, 1.0f, 10.f);
}

void SandboxApp::CreatePipelineState()
{
    RS::Shader shader;
    RS::Shader::Description shaderDesc{};
    shaderDesc.path = "Core/MeshShader.hlsl";
    shaderDesc.typeFlags = RS::Shader::TypeFlag::Pixel | RS::Shader::TypeFlag::Vertex;
    shader.Create(shaderDesc);

    // TODO: Remove this
    //{ // Test reflection
    //    ID3D12ShaderReflection* reflection = shader.GetReflection(RS::Shader::TypeFlag::Pixel);
    //
    //    D3D12_SHADER_DESC d12ShaderDesc{};
    //    DXCall(reflection->GetDesc(&d12ShaderDesc));
    //
    //    D3D12_SHADER_INPUT_BIND_DESC desc{};
    //    DXCall(reflection->GetResourceBindingDesc(0, &desc));
    //
    //    D3D12_SHADER_INPUT_BIND_DESC desc2{};
    //    DXCall(reflection->GetResourceBindingDesc(1, &desc2));
    //}

    CreateRootSignature();

    auto pDevice = RS::DX12Core3::Get()->GetD3D12Device();

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

        inputElementDescs.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        inputElementDescs.push_back({ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
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

    // TODO: Make it easier to make these by having some default states.
    // TODO: Also make keys for each PSO (shader, topology tyep, format, raster state, blend state, etc.)
    //      such that the renderer can have a SetShader function (Which basically sets the PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = inputLayoutDesc;
    psoDesc.pRootSignature = m_pRootSignature->GetRootSignature().Get();
    psoDesc.VS = GetShaderBytecode(shader.GetShaderBlob(RS::Shader::TypeFlag::Vertex, true));
    psoDesc.PS = GetShaderBytecode(shader.GetShaderBlob(RS::Shader::TypeFlag::Pixel, true));
    psoDesc.DS = GetShaderBytecode(shader.GetShaderBlob(RS::Shader::TypeFlag::Domain, true));
    psoDesc.HS = GetShaderBytecode(shader.GetShaderBlob(RS::Shader::TypeFlag::Hull, true));
    psoDesc.GS = GetShaderBytecode(shader.GetShaderBlob(RS::Shader::TypeFlag::Geometry, true));
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = true;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = true;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    psoDesc.DepthStencilState.StencilEnable = false;

    //psoDesc.DSVFormat
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO: Get this format from the render target.
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // TODO: Get this format from the render target.
    psoDesc.SampleDesc.Count = 1;
    //psoDesc.CachedPSO
    psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    psoDesc.NodeMask = 0; // Single GPU -> Set to 0.
    //psoDesc.StreamOutput;
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    //#ifdef RS_CONFIG_DEBUG
    //    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
    //#endif
    DXCall(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState)));

    shader.Release();
}

void SandboxApp::CreateRootSignature()
{
    // TODO: Have a main root signature for all shader to share?
    m_pRootSignature = std::make_shared<RS::RootSignature>(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    uint32 registerSpace = 0;
    uint32 currentShaderRegisterCBV = 0;
    uint32 currentShaderRegisterSRV = 0;
    uint32 currentShaderRegisterUAV = 0;
    uint32 currentShaderRegisterSampler = 0;

    const uint32 cbvRegSpace = 1;
    const uint32 srvRegSpace = 2;
    const uint32 uavRegSpace = 3;

    auto& rootSignature = *m_pRootSignature.get();
    rootSignature[RootParameter::PixelData].Constants(4 * 2, currentShaderRegisterCBV++, registerSpace);
    rootSignature[RootParameter::PixelData2].CBV(currentShaderRegisterCBV++, registerSpace);
    rootSignature[RootParameter::VertexData].CBV(currentShaderRegisterCBV++, registerSpace);

    // All bindless buffers, textures overlap using different spaces.
    // TODO: DynamicDescriptorHeap does not like when we have empty descriptors (not set) and not bindless too.
    rootSignature[RootParameter::Textures][0].SRV(2, 0, srvRegSpace);
    //rootSignature[RootParameter::ConstantBufferViews][0].CBV(1, 0, cbvRegSpace);
    //rootSignature[RootParameter::UnordedAccessViews][0].UAV(1, 0, uavRegSpace);

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

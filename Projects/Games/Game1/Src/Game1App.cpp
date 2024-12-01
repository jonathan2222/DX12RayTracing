#include "Game1App.h"

#include "Core/CorePlatform.h"
#include "DX12/NewCore/Shader.h"

#include "Maths/RSMatrix.h"

#include "Loaders/openfbx/FBXLoader.h"

//#define GLM_FORCE_LEFT_HANDED 
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"

#include "Core/Console.h"

RS_ADD_GLOBAL_CONSOLE_VAR(bool, "Game1App.debug1", g_Debug1, false, "A debug bool");
RS_ADD_GLOBAL_CONSOLE_VAR(float, "Game1App.translation.z", g_TranslationZ, 0.f, "Translation z");

Game1App::Game1App()
{
    Init();
}

Game1App::~Game1App()
{
}

void Game1App::FixedTick()
{
}

void Game1App::Tick(const RS::FrameStats& frameStats)
{
    {
        m_RenderTarget->UpdateSize();

        auto pCommandQueue = RS::DX12Core3::Get()->GetDirectCommandQueue();
        auto pCommandList = pCommandQueue->GetCommandList();

        pCommandList->SetRootSignature(m_pRootSignature);
        pCommandList->SetPipelineState(m_GraphicsPSO);

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

        //auto pRenderTargetDepthTexture = m_RenderTarget->GetAttachment(RS::AttachmentPoint::DepthStencil);
        //pCommandList->ClearDSV(pRenderTargetDepthTexture, D3D12_CLEAR_FLAG_DEPTH, pRenderTargetDepthTexture->GetClearValue()->DepthStencil.Depth, pRenderTargetDepthTexture->GetClearValue()->DepthStencil.Stencil);

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

        static bool sCameraIsActive = false;
        if (RS::Input::Get()->IsKeyClicked(RS::Key::C))
        {
            sCameraIsActive ^= 1;
            if (sCameraIsActive) RS::Input::Get()->LockMouse();
            else                 RS::Input::Get()->UnlockMouse();
        }
        //if (sCameraIsActive)
            m_Camera.Update(frameStats.frame.currentDT);
        
        float scale = 1.0f;
        vertexViewData.camera = g_Debug1 ? glm::identity<glm::mat4>() : glm::transpose(m_Camera.GetProjection());
        vertexViewData.transform =
        {
            scale, 0.f, 0.f, 0.f,
            0.f, scale, 0.f, 0.f,
            0.f, 0.f, scale, 0.0f,
            0.f, 0.f, 0.f, 1.f
        };
        vertexViewData.camera = vertexViewData.camera;
        vertexViewData.transform = vertexViewData.transform;
        vertexViewData.transform = glm::transpose(glm::translate(glm::vec3{ 0.f, 0.0f, g_TranslationZ }) * vertexViewData.transform);
        pCommandList->SetGraphicsDynamicConstantBuffer(RootParameter::VertexData, sizeof(vertexViewData), (void*)&vertexViewData);

        pCommandList->BindTexture(RootParameter::Textures, 0, m_NormalTexture);
        pCommandList->BindTexture(RootParameter::Textures, 1, m_NullTexture);

        pCommandList->DrawInstanced(m_NumVertices, 1, 0, 0);

        //pCommandList->SetGraphicsDynamicConstantBuffer(RootParameter::VertexData, sizeof(vertexViewData), (void*)&vertexViewData);

        // TODO: Change this to render to backbuffer instead of copy. Reason: If window gets resized this will not work.
        auto pTexture = m_RenderTarget->GetColorTextures()[0];
        pCommandList->CopyResource(RS::DX12Core3::Get()->GetSwapChain()->GetCurrentBackBuffer(), pTexture);

        pCommandQueue->ExecuteCommandList(pCommandList);
    }
}

void Game1App::Init()
{
    CreatePipelineState();

    //RS::Mesh* pMesh = RS::FBXLoader::Load("Sword2.fbx");
    //RS::Mesh* pMesh2 = RS::FBXLoader::Load("Suzanne.fbx");

    auto pCommandQueue = RS::DX12Core3::Get()->GetDirectCommandQueue();
    auto pCommandList = pCommandQueue->GetCommandList();

    //if (pMesh)
    //{
    //    delete pMesh;
    //    pMesh = nullptr;
    //}
    //
    //if (pMesh2)
    //{
    //    delete pMesh2;
    //    pMesh2 = nullptr;
    //}

    float scale = 1.0f;
    float aspectRatio = 1.0f;
    RS::Vertex triangleVertices[] =
    {
        { { -scale, +scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } }, // TL
        { { +scale, +scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }, // TR
        { { -scale, -scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }, // BL
    
        { { +scale, +scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }, // TR
        { { +scale, -scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } }, // BR
        { { -scale, -scale * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }  // BL
    };
    m_NumVertices = 6;
    const UINT vertexBufferSize2 = sizeof(triangleVertices);
    m_pVertexBufferResource = pCommandList->CreateVertexBufferResource(vertexBufferSize2, sizeof(RS::Vertex), "Vertex Buffer");
    pCommandList->UploadToBuffer(m_pVertexBufferResource, vertexBufferSize2, (void*)&triangleVertices[0]);

    // Texture
    {
        std::string texturePath = std::string("flyToYourDream.jpg");
        std::unique_ptr<RS::CorePlatform::Image> pImage = RS::CorePlatform::Get()->LoadImageData(texturePath, RS::RS_FORMAT_R8G8B8A8_UNORM, RS::CorePlatform::ImageFlag::FLIP_Y);
        m_NormalTexture = pCommandList->CreateTexture(pImage->width, pImage->height, pImage->pData, RS::DX12::GetDXGIFormat(pImage->format), "FlyToYTourDeam Texture Resource");
    }

    // Null Texture
    {
        std::string texturePath = std::string("NullTexture.png");
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

    //D3D12_CLEAR_VALUE clearValueDepth;
    //clearValueDepth.Format = DXGI_FORMAT_D32_FLOAT; // Only depth
    //clearValueDepth.DepthStencil.Depth = 0.0f;
    //clearValueDepth.DepthStencil.Stencil = 0;
    //auto pDepthTexture = pCommandList->CreateTexture(
    //    RS::Display::Get()->GetWidth(),
    //    RS::Display::Get()->GetHeight(),
    //    nullptr,
    //    clearValueDepth.Format,
    //    "Canvas Depth Texture",
    //    D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &clearValueDepth);
    //m_RenderTarget->SetAttachment(RS::AttachmentPoint::DepthStencil, pDepthTexture);

    RS::Display::Get()->AddOnSizeChangeCallback("Game1 RenderTarget", m_RenderTarget.get());

    uint64 fenceValue = pCommandQueue->ExecuteCommandList(pCommandList);

    // Wait for load to finish.
    pCommandQueue->WaitForFenceValue(fenceValue);

    float aspect = RS::Display::Get()->GetAspectRatio();
    m_Camera.Init(-10, 10, -10, 10, {0.f, 0.f, 1.f});
}

void Game1App::CreatePipelineState()
{
    RS::Shader shader;
    RS::Shader::Description shaderDesc{};
    shaderDesc.path = "Core/MeshShader.hlsl";
    shaderDesc.typeFlags = RS::Shader::TypeFlag::Pixel | RS::Shader::TypeFlag::Vertex;
    shader.Create(shaderDesc);

    CreateRootSignature();

    auto pDevice = RS::DX12Core3::Get()->GetD3D12Device();

    std::vector<RS::InputElementDesc> inputElementDescs;
    // TODO: Move this, and don't hardcode it!
    {
        RS::InputElementDesc inputElementDesc = {};
        inputElementDesc.SemanticName = "SV_POSITION";
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

    m_GraphicsPSO.SetDefaults();
    m_GraphicsPSO.SetInputLayout(inputElementDescs);
    m_GraphicsPSO.SetRootSignature(m_pRootSignature);
    m_GraphicsPSO.SetShader(&shader);
    m_GraphicsPSO.SetRTVFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
    m_GraphicsPSO.SetDSVFormat(DXGI_FORMAT_D32_FLOAT);
    D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.FrontCounterClockwise = false;
    m_GraphicsPSO.SetRasterizerState(rasterizerDesc);
    m_GraphicsPSO.Create();

    shader.Release();
}

void Game1App::CreateRootSignature()
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
    // TODO: Support bindless descriptors!
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

    rootSignature.Bake("Game1_RootSignature");
}

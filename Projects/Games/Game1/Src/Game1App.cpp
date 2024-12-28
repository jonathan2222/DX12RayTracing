#include "Game1App.h"

#include "Core/CorePlatform.h"
#include "DX12/NewCore/Shader.h"

#include "Maths/RSMatrix.h"

#include "Loaders/openfbx/FBXLoader.h"

//#define GLM_FORCE_LEFT_HANDED 
#include "Maths/GLMDefines.h"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"

#include "Core/Console.h"

RS_ADD_GLOBAL_CONSOLE_VAR(bool, "Game1App.debug1", g_Debug1, false, "A debug bool");
RS_ADD_GLOBAL_CONSOLE_VAR(float, "Game1App.translation.x", g_TranslationX, 0.f, "Translation x");
RS_ADD_GLOBAL_CONSOLE_VAR(float, "Game1App.translation.y", g_TranslationY, 0.f, "Translation y");
RS_ADD_GLOBAL_CONSOLE_VAR(float, "Game1App.translation.z", g_TranslationZ, 0.f, "Translation z");
RS_ADD_GLOBAL_CONSOLE_VAR(uint, "Game1App.instanceCount", g_InstanceCount, 10, "Instance Count");

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
    m_RenderTarget->UpdateSize();

    static bool sCameraIsActive = false;
    if (RS::Input::Get()->IsKeyClicked(RS::Key::C))
    {
        sCameraIsActive ^= 1;
        if (sCameraIsActive) RS::Input::Get()->LockMouse();
        else                 RS::Input::Get()->UnlockMouse();
    }
    //if (sCameraIsActive)
    m_Camera.Update(frameStats.frame.currentDT);

    // == Update ==

    // Update enemy positions
    for (Entity& ent : m_Enemies)
        ent.m_Position += ent.m_Velocity * frameStats.frame.currentDT;

    auto pCommandQueue = RS::DX12Core3::Get()->GetDirectCommandQueue();
    auto pCommandList = pCommandQueue->GetCommandList();

    // Spawn new enemies
    static float time = 0.f;
    time += frameStats.frame.currentDT;
    if (time > 1.f)
    {
        float r = RS::Utils::Rand01();
        Entity::Type type = Entity::Type::EASY_ENEMY;
        if (r < 0.6)
            type = Entity::Type::EASY_ENEMY;
        else
            type = Entity::Type::NORMAL_ENEMY;
        SpawnEnemy(type, 4.f, pCommandList);
        time = 0.f;
    }

    // == Render ==

    UpdateEntitiesInstanceData(pCommandList);

    DrawEntites(frameStats, pCommandList);

    DrawPlayer(pCommandList);

    // TODO: Change this to render to backbuffer instead of copy. Reason: If window gets resized this will not work.
    auto pTexture = m_RenderTarget->GetColorTextures()[0];
    pCommandList->CopyResource(RS::DX12Core3::Get()->GetSwapChain()->GetCurrentBackBuffer(), pTexture);

    pCommandQueue->ExecuteCommandList(pCommandList);
}

void Game1App::Init()
{
    float aspect = RS::Display::Get()->GetAspectRatio();
    m_WorldSize = glm::vec2(20 * aspect, 20);
    m_Camera.Init(-m_WorldSize.x * .5f, m_WorldSize.x * .5f, -m_WorldSize.y*.5, m_WorldSize.y*.5f, -10.f, 10.f, { 0.f, 0.f, 1.f });

    CreatePipelineStateEntities();
    CreatePipelineStatePlayer();

    auto pCommandQueue = RS::DX12Core3::Get()->GetDirectCommandQueue();
    auto pCommandList = pCommandQueue->GetCommandList();

    struct Vertex
    {
        glm::vec3 position;
        glm::vec2 uv;
    };
    Vertex triangleVertices[] =
    {
        { { -1.f, +1.f, 0.0f }, { 0.0f, 1.0f } }, // TL
        { { +1.f, +1.f, 0.0f }, { 1.0f, 1.0f } }, // TR
        { { -1.f, -1.f, 0.0f }, { 0.0f, 0.0f } }, // BL
    
        { { +1.f, +1.f, 0.0f }, { 1.0f, 1.0f } }, // TR
        { { +1.f, -1.f, 0.0f }, { 1.0f, 0.0f } }, // BR
        { { -1.f, -1.f, 0.0f }, { 0.0f, 0.0f } }  // BL
    };
    m_NumVertices = 6;
    const UINT vertexBufferSize2 = sizeof(triangleVertices);
    m_pVertexBufferResource = pCommandList->CreateVertexBufferResource(vertexBufferSize2, sizeof(Vertex), "Vertex Buffer");
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

    // Instance data
    //UpdateInstanceData(pCommandList, g_InstanceCount);

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

    RS::Display::Get()->AddOnSizeChangeCallback("Game1 RenderTarget", m_RenderTarget.get());

    uint64 fenceValue = pCommandQueue->ExecuteCommandList(pCommandList);

    // Wait for load to finish.
    pCommandQueue->WaitForFenceValue(fenceValue);
}

void Game1App::CreatePipelineStateEntities()
{
    RS::Shader shader;
    RS::Shader::Description shaderDesc{};
    shaderDesc.path = "Game/EntityShader.hlsl";
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

        inputElementDescs.push_back({ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    }

    m_EntitiesPSO.SetDefaults();
    m_EntitiesPSO.SetInputLayout(inputElementDescs);
    m_EntitiesPSO.SetRootSignature(m_pRootSignature);
    m_EntitiesPSO.SetShader(&shader);
    m_EntitiesPSO.SetRTVFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
    //m_EntitiesPSO.SetDSVFormat(DXGI_FORMAT_UNKNOWN);
    m_EntitiesPSO.SetDSVFormat(DXGI_FORMAT_D32_FLOAT);
    D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.FrontCounterClockwise = false;
    m_EntitiesPSO.SetRasterizerState(rasterizerDesc);
    m_EntitiesPSO.Create();

    shader.Release();
}

void Game1App::CreatePipelineStatePlayer()
{
    RS::Shader shader;
    RS::Shader::Description shaderDesc{};
    shaderDesc.path = "Game/PlayerShader.hlsl";
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

        inputElementDescs.push_back({ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    }

    m_PlayerPSO.SetDefaults();
    m_PlayerPSO.SetInputLayout(inputElementDescs);
    m_PlayerPSO.SetRootSignature(m_pRootSignature);
    m_PlayerPSO.SetShader(&shader);
    m_PlayerPSO.SetRTVFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
    //m_PlayerPSO.SetDSVFormat(DXGI_FORMAT_UNKNOWN);
    m_PlayerPSO.SetDSVFormat(DXGI_FORMAT_D32_FLOAT);
    D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.FrontCounterClockwise = false;
    m_PlayerPSO.SetRasterizerState(rasterizerDesc);
    m_PlayerPSO.Create();

    shader.Release();
}

void Game1App::CreateRootSignature()
{
    // TODO: Have a main root signature for all shader to share?
    m_pRootSignature = std::make_shared<RS::RootSignature>(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    uint32 currentShaderRegisterCBV = 0;
    uint32 currentShaderRegisterSRV = 0;
    uint32 currentShaderRegisterUAV = 0;
    uint32 currentShaderRegisterSampler = 0;

    const uint32 cbvRegSpace = 0;
    const uint32 srvRegSpace = 1;
    const uint32 uavRegSpace = 2;
    const uint32 samplerRegSpace = 0;

    auto& rootSignature = *m_pRootSignature.get();
    rootSignature[RootParameter::CBVs].CBV(currentShaderRegisterCBV++, cbvRegSpace);

    // All bindless buffers, textures overlap using different spaces.
    // TODO: Support bindless descriptors!
    rootSignature[RootParameter::SRVs][0].SRV(1, 0, srvRegSpace);

    //{
    //    CD3DX12_STATIC_SAMPLER_DESC samplerDesc{};
    //    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    //    samplerDesc.Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT; // Point sample for min, max, mag, mip.
    //    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    //    samplerDesc.MaxAnisotropy = 16;
    //    samplerDesc.RegisterSpace = samplerRegSpace;
    //    samplerDesc.ShaderRegister = currentShaderRegisterSampler++;
    //    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    //    samplerDesc.MipLODBias = 0;
    //    samplerDesc.MinLOD = 0.0f;
    //    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    //    rootSignature.AddStaticSampler(samplerDesc);
    //}

    rootSignature.Bake("Game1_RootSignature");
}

void Game1App::DrawEntites(const RS::FrameStats& frameStats, std::shared_ptr<RS::CommandList> pCommandList)
{
    if (m_InstanceData.empty())
        return;

    pCommandList->SetRootSignature(m_pRootSignature);
    pCommandList->SetPipelineState(m_EntitiesPSO);

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
    pCommandList->ClearDSV(pRenderTargetDepthTexture, D3D12_CLEAR_FLAG_DEPTH, pRenderTargetDepthTexture->GetClearValue()->DepthStencil.Depth, pRenderTargetDepthTexture->GetClearValue()->DepthStencil.Stencil);

    pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->SetVertexBuffers(0, m_pVertexBufferResource);

    struct VertexConstantBufferData
    {
        glm::mat4 camera;
    } vertexViewData;

    float scale = 1.0f;
    vertexViewData.camera = g_Debug1 ? glm::identity<glm::mat4>() : glm::transpose(m_Camera.GetProjection());
    vertexViewData.camera = vertexViewData.camera;
    pCommandList->SetGraphicsDynamicConstantBuffer(RootParameter::CBVs, sizeof(vertexViewData), (void*)&vertexViewData);

    pCommandList->BindBuffer(RootParameter::SRVs, 0, m_InstanceBuffer, &m_InstanceDataSRVDesc);

    pCommandList->DrawInstanced(m_NumVertices, m_InstanceDataSRVDesc.Buffer.NumElements, 0, 0);
}

void Game1App::DrawPlayer(std::shared_ptr<RS::CommandList> pCommandList)
{
    pCommandList->SetRootSignature(m_pRootSignature);
    pCommandList->SetPipelineState(m_PlayerPSO);

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
    
    pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->SetVertexBuffers(0, m_pVertexBufferResource);

    float scale = 2.0f;
    struct VertexConstantBufferData
    {
        glm::mat4 camera;
    } vertexViewData;

    glm::vec2 mousePos = RS::Input::Get()->GetMousePos() / RS::Display::Get()->GetSize();
    mousePos.y = 1.f - mousePos.y;
    mousePos *= m_WorldSize;
    mousePos -= m_WorldSize * 0.5f;
    vertexViewData.camera = g_Debug1 ? glm::identity<glm::mat4>() :
        glm::transpose(m_Camera.GetProjection() * glm::translate(glm::vec3(mousePos, 0.f)) * glm::scale(glm::vec3(scale)));
    vertexViewData.camera = vertexViewData.camera;
    pCommandList->SetGraphicsDynamicConstantBuffer(RootParameter::CBVs, sizeof(vertexViewData), (void*)&vertexViewData);

    pCommandList->DrawInstanced(m_NumVertices, 1, 0, 0);
}

void Game1App::SpawnEnemy(Entity::Type type, float initialSpeed, std::shared_ptr<RS::CommandList> pCommandList)
{
    // == Add enemy ==
    
    // Spawn the enemy outside the view, pick position from a random angle around the unit circle.
    float angle = RS::Utils::RandRad();
    glm::vec2 spawnPoint(std::cos(angle), std::sin(angle));
    spawnPoint *= std::max(m_WorldSize.x, m_WorldSize.y);
    spawnPoint *= 1.05f; // Add extra distance such that we cannot see them being spawned in.
    
    // Pick the velocity to point towards an area around the center of the screen.
    angle = RS::Utils::RandRad();
    glm::vec2 velocity(std::cos(angle), std::sin(angle));
    velocity *= std::min(m_WorldSize.x, m_WorldSize.y) * 0.5f;
    velocity = glm::normalize(velocity - spawnPoint) * initialSpeed;
    m_Enemies.emplace_back(type, spawnPoint, velocity);

    // Add entity to instance data
    InstanceData instanceData;
    instanceData.transform = glm::transpose(glm::translate(glm::vec3(spawnPoint, 0.f)));
    instanceData.type = (uint)type;
    instanceData.padding0 = instanceData.padding1 = instanceData.padding2 = 255;
    m_InstanceData.push_back(instanceData);

    // Update GPU buffer
    uint newCount = (uint)m_InstanceData.size();
    ResizeEntitiesInstanceData(newCount, pCommandList, true);
}

void Game1App::ResizeEntitiesInstanceData(uint newCount, std::shared_ptr<RS::CommandList> pCommandList, bool updateData)
{
    uint sizeInBytes = (uint)(m_InstanceData.size() * sizeof(InstanceData));
    m_InstanceBuffer = pCommandList->CreateBufferResource(sizeInBytes, RS::Utils::Format("Instance Data Buffer {}", newCount));
    
    if (updateData)
        pCommandList->UploadToBuffer(m_InstanceBuffer, sizeInBytes, (void*)&m_InstanceData[0]);

    m_InstanceDataSRVDesc = {};
    m_InstanceDataSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    m_InstanceDataSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    m_InstanceDataSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_InstanceDataSRVDesc.Buffer.FirstElement = 0;
    m_InstanceDataSRVDesc.Buffer.NumElements = newCount;
    m_InstanceDataSRVDesc.Buffer.StructureByteStride = sizeof(InstanceData);
    m_InstanceDataSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
}

void Game1App::UpdateEntitiesInstanceData(std::shared_ptr<RS::CommandList> pCommandList)
{
    if (m_InstanceData.empty())
        return;

    RS_ASSERT(m_InstanceData.size() == m_Enemies.size(), "Cannot update buffer if the data sizes differ!");

    // Transfer enemy data
    for (uint i = 0; i < (uint)m_InstanceData.size(); i++)
    {
        Entity& enemy = m_Enemies[i];
        InstanceData& instance = m_InstanceData[i];
        instance.transform = glm::transpose(glm::translate(glm::vec3(enemy.m_Position, 0.f)));
        instance.type = (uint)enemy.m_Type;
    }

    // Upload the new data to the GPU
    uint sizeInBytes = (uint)(m_InstanceData.size() * sizeof(InstanceData));
    pCommandList->UploadToBuffer(m_InstanceBuffer, sizeInBytes, (void*)&m_InstanceData[0]);
}

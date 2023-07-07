#include "PreCompiled.h"
#include "DX12Core3.h"

#include "Core/EngineLoop.h"

// TODO: Remove/Move these!
#include "Render/ImGuiRenderer.h"
#include "GUI/LogNotifier.h"
#include "Core/Console.h"

#include "DX12/NewCore/Shader.h"

RS::DX12Core3::~DX12Core3()
{
}

void RS::DX12Core3::Flush()
{
    m_pDirectCommandQueue->Flush();
}

void RS::DX12Core3::WaitForGPU()
{
    Flush();
    uint64 fenceValue = m_pDirectCommandQueue->Signal();
    m_pDirectCommandQueue->WaitForFenceValue(fenceValue);

    for (uint32 frameIndex = 0; frameIndex < FRAME_BUFFER_COUNT; ++frameIndex)
        ReleasePendingResourceRemovals(frameIndex);
}

RS::DescriptorAllocator* RS::DX12Core3::GetDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    return m_pDescriptorHeaps[type].get();
}

RS::DescriptorAllocation RS::DX12Core3::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    return GetDescriptorAllocator(type)->Allocate(1);
}

void RS::DX12Core3::AddToLifetimeTracker(uint64 id, const std::string& name)
{
    std::lock_guard<std::mutex> lock(s_ResourceLifetimeTrackingMutex);
    s_ResourceLifetimeTrackingData[id] = LifetimeStats(name);
}

void RS::DX12Core3::RemoveFromLifetimeTracker(uint64 id)
{
    std::lock_guard<std::mutex> lock(s_ResourceLifetimeTrackingMutex);
    s_ResourceLifetimeTrackingData.erase(id);
}

void RS::DX12Core3::SetNameOfLifetimeTrackedResource(uint64 id, const std::string& name)
{
    std::lock_guard<std::mutex> lock(s_ResourceLifetimeTrackingMutex);
    strcpy_s(s_ResourceLifetimeTrackingData[id].name, name.c_str());
}

void RS::DX12Core3::LogLifetimeTracker()
{
    if (LaunchArguments::Contains(LaunchParams::logResources))
    {
        LOG_DEBUG("Resource Lifetime tracker:");
        std::lock_guard<std::mutex> lock(s_ResourceLifetimeTrackingMutex);
        uint64 currentTime = Timer::GetCurrentTimeSeconds();
        uint32 index = 0;
        for (auto& entry : s_ResourceLifetimeTrackingData)
        {
            const uint64 id = entry.first;
            LifetimeStats& stats = entry.second;
            LOG_DEBUG("[{}]\tID: {} [{}] AliveTime: {} s", index, id, stats.name, currentTime - stats.startTime);
            ++index;
        }

        uint32 frameIndex = RS::DX12Core3::Get()->GetCurrentFrameIndex();
        std::array<uint32, FRAME_BUFFER_COUNT> numPendingRemovals = RS::DX12Core3::Get()->GetNumberOfPendingPremovals();
        LOG_DEBUG("Penging Removals:");
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; ++i)
            LOG_DEBUG("{}\t[{}] -> {}", frameIndex == i ? "x" : "", i, numPendingRemovals[i]);
        LOG_DEBUG("Global resource states: {}", RS::ResourceStateTracker::GetNumberOfGlobalResources());
    }
}

std::array<uint32, FRAME_BUFFER_COUNT> RS::DX12Core3::GetNumberOfPendingPremovals()
{
    std::lock_guard<std::mutex> lock(m_ResourceRemovalMutex);
    std::array<uint32, FRAME_BUFFER_COUNT> arrayData;
    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; ++i)
        arrayData[i] = (uint32)m_PendingResourceRemovals[i].size();
    return arrayData;
}

RS::DX12Core3::DX12Core3()
    : m_CurrentFrameIndex(0)
    , m_FenceValues{ 0 }
{
    const D3D_FEATURE_LEVEL d3dMinFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    const DX12::DXGIFlags dxgiFlags = DX12::DXGIFlag::REQUIRE_TEARING_SUPPORT;
    m_Device.Init(d3dMinFeatureLevel, dxgiFlags);
}

RS::DX12Core3* RS::DX12Core3::Get()
{
    static std::unique_ptr<DX12Core3> pDX12Core(new DX12Core3());
    return pDX12Core.get();
}

void RS::DX12Core3::Init(HWND window, int width, int height)
{
    m_pDirectCommandQueue = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);

    for (uint32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        m_pDescriptorHeaps[i] = std::make_unique<DescriptorAllocator>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i), 256);

    // TODO: This should be able to be outside this calss. Think what happens when we have multiple swap chains.
    m_pSwapChain = std::make_unique<SwapChain>(window, width, height, m_Device.GetDXGIFlags());

    // TODO: Remove/Move these!
    {
        // ImGui
        ImGuiRenderer::Get()->Init();
        
        m_ResizeTexturePipelineState = nullptr;
    }
}

void RS::DX12Core3::Release()
{
    m_ResizeTextureRootSignature.reset();
    if (m_ResizeTexturePipelineState)
    {
        m_ResizeTexturePipelineState->Release();
        m_ResizeTexturePipelineState = nullptr;
    }

    m_pSwapChain.reset();
    ImGuiRenderer::Get()->Release();
    WaitForGPU();
    DX12Core3::LogLifetimeTracker();
    m_Device.Release();
}

void RS::DX12Core3::FreeResource(Microsoft::WRL::ComPtr<ID3D12Resource> pResource)
{
    uint32 frameIndex = GetCurrentFrameIndex();
    std::lock_guard<std::mutex> lock(m_ResourceRemovalMutex);
    m_PendingResourceRemovals[frameIndex].push_back(pResource);
}

bool RS::DX12Core3::WindowSizeChanged(uint32 width, uint32 height, bool isFullscreen, bool windowed, bool minimized)
{
    bool isWindowVisible = width != 0 && height != 0 && !minimized;
    if (!isWindowVisible)
        return false;

    LOG_WARNING("Resize to: ({}, {}) isFullscreen: {}, windowed: {}", width, height, isFullscreen, windowed);

    m_pSwapChain->Resize(width, height, isFullscreen);
    ImGuiRenderer::Get()->Resize();
    return true;
}

void RS::DX12Core3::Render()
{
    const uint32 frameIndex = GetCurrentFrameIndex();
    ReleasePendingResourceRemovals(frameIndex);

    // Clear swap chain's back buffer.
    auto pCommandQueue = GetDirectCommandQueue();
    auto pCommandList = pCommandQueue->GetCommandList();
    float pClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    pCommandList->ClearTexture(m_pSwapChain->GetCurrentBackBuffer(), pClearColor);
    pCommandQueue->ExecuteCommandList(pCommandList);

    {
        // TODO: Move these!
        ImGuiRenderer::Get()->Draw([&]() {
            Console::Get()->Render();

            // Render toasts on top of everything, at the end of your code!
            // You should push style vars here
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f); // Round borders
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f)); // Background color
            ImGui::RenderNotifications(); // <-- Here we render all notifications
            ImGui::PopStyleVar(1); // Don't forget to Pop()
            ImGui::PopStyleColor(1);
        });

        // ImGui
        ImGuiRenderer::Get()->Render();
    }

    // Frame index is the same as the back buffer index.
    m_CurrentFrameIndex = m_pSwapChain->Present(nullptr);
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
        //resource->Release();
        resource.Reset();
    }
}

void RS::DX12Core3::ResizeTexture(const std::shared_ptr<Texture>& pTexture, uint32 newWidth, uint32 newHeight)
{

    std::string copyName = pTexture->GetName() + " Copy";
    auto textureDesc = pTexture->GetD3D12ResourceDesc();
    std::shared_ptr<Texture> pSRVTexture;

    {
        auto pCommandQueue = GetDirectCommandQueue();
        auto pCommandList = pCommandQueue->GetCommandList();
        pSRVTexture = pCommandList->CreateTexture(textureDesc.Width, textureDesc.Height, nullptr, textureDesc.Format, copyName);
        pCommandList->CopyResource(pSRVTexture, pTexture);
        pCommandQueue->ExecuteCommandList(pCommandList);
    }

    auto pCommandQueue = GetDirectCommandQueue();
    auto pCommandList = pCommandQueue->GetCommandList();

    std::shared_ptr<RenderTarget> pRenderTarget = std::make_shared<RenderTarget>();
    pTexture->Resize(newWidth, newHeight, textureDesc.DepthOrArraySize);
    pRenderTarget->SetAttachment(AttachmentPoint::Color0, pTexture);

    //float pClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    //pCommandList->ClearTexture(pTexture, pClearColor);

    if (m_ResizeTexturePipelineState == nullptr)
    {
        RS::Shader shader;
        RS::Shader::Description shaderDesc{};
        shaderDesc.path = "Core/ResizeShader.hlsl";
        shaderDesc.typeFlags = RS::Shader::TypeFlag::Pixel | RS::Shader::TypeFlag::Vertex;
        shader.Create(shaderDesc);

        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        m_ResizeTextureRootSignature = std::make_shared<RootSignature>(rootSignatureFlags);

        auto& rootSignature = *m_ResizeTextureRootSignature.get();
        rootSignature[0][0].SRV(1, 0);
        rootSignature[0].SetVisibility(D3D12_SHADER_VISIBILITY_PIXEL);

        {
            CD3DX12_STATIC_SAMPLER_DESC staticSampler{};
            staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSampler.MipLODBias = 0.f;
            staticSampler.MaxAnisotropy = 0;
            staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            staticSampler.MinLOD = 0.f;
            staticSampler.MaxLOD = 0.f;
            staticSampler.ShaderRegister = 0;
            staticSampler.RegisterSpace = 0;
            staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            rootSignature.AddStaticSampler(staticSampler);
        }

        rootSignature.Bake();

        auto pDevice = DX12Core3::Get()->GetD3D12Device();

        auto GetShaderBytecode = [&](IDxcBlob* shaderObject)->CD3DX12_SHADER_BYTECODE {
            if (shaderObject)
                return CD3DX12_SHADER_BYTECODE(shaderObject->GetBufferPointer(), shaderObject->GetBufferSize());
            return CD3DX12_SHADER_BYTECODE(nullptr, 0);
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        memset(&psoDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        psoDesc.InputLayout = { nullptr, 0 };
        psoDesc.pRootSignature = m_ResizeTextureRootSignature->GetRootSignature().Get();
        psoDesc.VS = GetShaderBytecode(shader.GetShaderBlob(Shader::TypeFlag::Vertex, true));
        psoDesc.PS = GetShaderBytecode(shader.GetShaderBlob(Shader::TypeFlag::Pixel, true));
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.NodeMask = 0; // Single GPU -> Set to 0.
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        // Create the blending setup
        {
            D3D12_BLEND_DESC& desc = psoDesc.BlendState;
            desc.AlphaToCoverageEnable = false;
            desc.RenderTarget[0].BlendEnable = true;
            desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        }

        // Create the rasterizer state
        {
            D3D12_RASTERIZER_DESC& desc = psoDesc.RasterizerState;
            desc.FillMode = D3D12_FILL_MODE_SOLID;
            desc.CullMode = D3D12_CULL_MODE_NONE;
            desc.FrontCounterClockwise = FALSE;
            desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
            desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
            desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
            desc.DepthClipEnable = true;
            desc.MultisampleEnable = FALSE;
            desc.AntialiasedLineEnable = FALSE;
            desc.ForcedSampleCount = 0;
            desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        }

        // Create depth-stencil State
        {
            D3D12_DEPTH_STENCIL_DESC& desc = psoDesc.DepthStencilState;
            desc.DepthEnable = false;
            desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            desc.StencilEnable = false;
            desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            desc.BackFace = desc.FrontFace;
        }

        DXCall(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_ResizeTexturePipelineState)));

        shader.Release();
    }

    pCommandList->SetRootSignature(m_ResizeTextureRootSignature);
    pCommandList->SetPipelineState(m_ResizeTexturePipelineState);
    pCommandList->SetRenderTarget(pRenderTarget);

    pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    D3D12_VIEWPORT viewport{};
    viewport.MaxDepth = 0;
    viewport.MinDepth = 1;
    viewport.TopLeftX = viewport.TopLeftY = 0;
    viewport.Width = newWidth;
    viewport.Height = newHeight;
    pCommandList->SetViewport(viewport);

    D3D12_RECT scissorRect{};
    scissorRect.left = scissorRect.top = 0;
    scissorRect.right = viewport.Width;
    scissorRect.bottom = viewport.Height;
    pCommandList->SetScissorRect(scissorRect);

    pCommandList->BindTexture(0, 0, pSRVTexture);
    pCommandList->DrawInstanced(3, 1, 0, 0);

    pCommandQueue->ExecuteCommandList(pCommandList);
}

#include "PreCompiled.h"
#include "DXDisplay.h"

#include "DXCore.h"
#include "Graphics/RenderCore.h"

#include "DX12/Final/DXCommandContext.h"
#include "DX12/Final/DXShader.h"

RS::DX12::DXDisplay::DXDisplay()
    : m_PresentSDRPSO(L"Core: PresentSDR")
    , m_ScalePresentSDRPSO(L"Core: ScalePresentSDR")
    , m_BlendUIPSO(L"Core: BlendUI")
{
}

void RS::DX12::DXDisplay::Init(HWND hWnd, uint32 width, uint32 height, DXGI_FORMAT format)
{
    m_Width = width;
    m_Height = height;
    m_SwapChainFormat = format;

	DXDevice* pDevice = DXCore::GetDXDevice();

    DX12_FACTORY_PTR pFactory = pDevice->GetDXGIFactory();

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = format;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
    fsSwapChainDesc.Windowed = TRUE;

    DXCall(pFactory->CreateSwapChainForHwnd(
        DXCore::GetCommandListManager()->GetCommandQueue(),
        hWnd,
        &swapChainDesc,
        &fsSwapChainDesc,
        nullptr,
        &m_pSwapChain1));

    // Check and prepare for HDR support.
    {
        IDXGISwapChain4* swapChain = (IDXGISwapChain4*)m_pSwapChain1;
        ComPtr<IDXGIOutput> output;
        ComPtr<IDXGIOutput6> output6;
        DXGI_OUTPUT_DESC1 outputDesc;
        UINT colorSpaceSupport;

        // Query support for ST.2084 on the display and set the color space accordingly
        if (SUCCEEDED(swapChain->GetContainingOutput(&output)) && SUCCEEDED(output.As(&output6)) &&
            SUCCEEDED(output6->GetDesc1(&outputDesc)) && outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 &&
            SUCCEEDED(swapChain->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &colorSpaceSupport)) &&
            (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) &&
            SUCCEEDED(swapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)))
        {
            m_EnableHDROutput = true;
        }
    }

    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
    {
        ComPtr<ID3D12Resource> displayPlane;
        DXCall(m_pSwapChain1->GetBuffer(i, IID_PPV_ARGS(&displayPlane)));
        m_DisplayPlanes[i].CreateFromSwapChain(L"Primary SwapChain Buffer", displayPlane.Detach());
    }

    m_PresentRootSignature.Reset(4, 2);
    m_PresentRootSignature[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
    m_PresentRootSignature[1].InitAsConstants(0, 6, D3D12_SHADER_VISIBILITY_ALL);
    m_PresentRootSignature[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
    m_PresentRootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
    m_PresentRootSignature.InitStaticSampler(0, RenderCore::SamplerLinearClampDesc);
    m_PresentRootSignature.InitStaticSampler(1, RenderCore::SamplerPointClampDesc);
    m_PresentRootSignature.Finalize(L"Present");

    DXShader::Description shaderDesc{};
    shaderDesc.isInternalPath = true;
    shaderDesc.typeFlags = DXShader::TypeFlag::Pixel;
    shaderDesc.path = "Core/BufferCopyPS.hlsl";
    DXShader bufferCopyPSShader;
    bufferCopyPSShader.Create(shaderDesc);
    IDxcBlob* pBufferCopyPSShaderBlob = bufferCopyPSShader.GetShaderBlob(shaderDesc.typeFlags);

    shaderDesc.path = "Core/ScreenQuadPresentVS.hlsl";
    shaderDesc.typeFlags = DXShader::TypeFlag::Vertex;
    DXShader screenQuadPresentVSShader;
    screenQuadPresentVSShader.Create(shaderDesc);
    IDxcBlob* pScreenQuadPresentVSShaderBlob = screenQuadPresentVSShader.GetShaderBlob(shaderDesc.typeFlags);

    // Initialize PSOs
    m_BlendUIPSO.SetRootSignature(m_PresentRootSignature);
    m_BlendUIPSO.SetRasterizerState(RenderCore::RasterizerTwoSided);
    m_BlendUIPSO.SetBlendState(RenderCore::BlendPreMultiplied);
    m_BlendUIPSO.SetDepthStencilState(RenderCore::DepthStateDisabled);
    m_BlendUIPSO.SetSampleMask(0xFFFFFFFF);
    m_BlendUIPSO.SetInputLayout(0, nullptr);
    m_BlendUIPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    m_BlendUIPSO.SetVertexShader(pScreenQuadPresentVSShaderBlob->GetBufferPointer(), pScreenQuadPresentVSShaderBlob->GetBufferSize());
    m_BlendUIPSO.SetPixelShader(pBufferCopyPSShaderBlob->GetBufferPointer(), pBufferCopyPSShaderBlob->GetBufferSize());
    m_BlendUIPSO.SetRenderTargetFormat(m_SwapChainFormat, DXGI_FORMAT_UNKNOWN);
    m_BlendUIPSO.Finalize();

#define CreatePSO(ObjName, shaderDesc) \
    { \
        ObjName = m_BlendUIPSO; \
        shader.Create(shaderDesc); \
        ObjName.SetBlendState(RenderCore::BlendDisable); \
        IDxcBlob* pShaderBlob = shader.GetShaderBlob(shaderDesc.typeFlags); \
        ObjName.SetPixelShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize()); \
        ObjName.Finalize(); \
        shader.Release(); \
    }

    DXShader shader;
    shaderDesc.path = "Core/SDRPresentShader.hlsl";
    shaderDesc.typeFlags = DXShader::TypeFlag::Pixel;
    CreatePSO(m_PresentSDRPSO, shaderDesc);
    shaderDesc.defines = { "NEEDS_SCALING" };
    CreatePSO(m_ScalePresentSDRPSO, shaderDesc);

#undef CreatePSO

    bufferCopyPSShader.Release();
    screenQuadPresentVSShader.Release();

    m_CurrentBuffer = 0;
    m_FrameIndex = 0;
    m_FrameStartTick = 0;
}

void RS::DX12::DXDisplay::Remove()
{
    RS_ASSERT(m_pSwapChain1);

    m_pSwapChain1->SetFullscreenState(FALSE, nullptr);
    m_pSwapChain1->Release();

    for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
        m_DisplayPlanes[i].Destroy();
}

void RS::DX12::DXDisplay::Present(DXColorBuffer* pBase)
{
    RS_ASSERT(pBase != nullptr);

    //if(m_EnableHDROutput)
    //    PresentHDR(pBase);
    //else
        PresentSDR(pBase);

    uint32 presentInterval = m_EnableVSync ? std::min(4u, (uint32)std::roundf(m_FrameTime * 60.0f)) : 0u;
    m_pSwapChain1->Present(presentInterval, 0);

    m_CurrentBuffer = (m_CurrentBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;

    uint64 currentTick = Timer::GetCurrentTick();

    if (m_EnableVSync)
    {
        // With VSync enabled, the time step between frames becomes a multiple of 16.666 ms.  We need
        // to add logic to vary between 1 and 2 (or 3 fields).  This delta time also determines how
        // long the previous frame should be displayed (i.e. the present interval.)
        m_FrameTime = (m_LimitTo30Hz ? 2.0f : 1.0f) / 60.0f;
        //if (s_DropRandomFrames)
        //{
        //    if (std::rand() % 50 == 0)
        //        s_FrameTime += (1.0f / 60.0f);
        //}
    }
    else
    {
        // When running free, keep the most recent total frame time as the time step for
        // the next frame simulation.  This is not super-accurate, but assuming a frame
        // time varies smoothly, it should be close enough.
        m_FrameTime = (float)Timer::CalcTimeIntervalInSeconds(m_FrameStartTick, currentTick);
    }

    m_FrameStartTick = currentTick;
    ++m_FrameIndex;
}

void RS::DX12::DXDisplay::Resize(uint32 width, uint32 height)
{
    DXCore::GetCommandListManager()->IdleGPU();

    m_Width = width;
    m_Height = height;

    RS_LOG_INFO("Changing display resolution to {}x{}", width, height);

    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
        m_DisplayPlanes[i].Destroy();

    RS_ASSERT(m_pSwapChain1 != nullptr);
    DXCall(m_pSwapChain1->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, width, height, m_SwapChainFormat, 0));

    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
    {
        ComPtr<ID3D12Resource> displayPlane;
        DXCall(m_pSwapChain1->GetBuffer(i, IID_PPV_ARGS(&displayPlane)));
        m_DisplayPlanes[i].CreateFromSwapChain(L"Primary SwapChain Buffer", displayPlane.Detach());
    }

    m_CurrentBuffer = 0;

    DXCore::GetCommandListManager()->IdleGPU();

    // TODO: Resize display dependent buffers!
}

void RS::DX12::DXDisplay::PresentSDR(DXColorBuffer* pBase)
{
    DXGraphicsContext& context = DXGraphicsContext::Begin(L"Present");

    context.SetRootSignature(m_PresentRootSignature);
    context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // We're going to be reading these buffers to write to the swap chain buffer(s)
    context.TransitionResource(*pBase, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    context.SetDynamicDescriptor(0, 0, pBase->GetSRV());

    // Render texture to the swapchain buffer
    context.SetPipelineState(m_PresentSDRPSO);
    context.TransitionResource(m_DisplayPlanes[m_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
    context.SetRenderTarget(m_DisplayPlanes[m_CurrentBuffer].GetRTV());
    context.SetViewportAndScissor(0, 0, m_Width, m_Height);
    context.Draw(3);

    context.TransitionResource(m_DisplayPlanes[m_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);

    // Close the final context to be executed before frame present.
    context.Finish();
}

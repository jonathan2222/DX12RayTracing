#include "PreCompiled.h"
#include "Dx12Core.h"

#include "DX12/Factory.h"
#include "Utils/Utils.h"

using namespace RS;

namespace
{
    inline DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt)
    {
        switch (fmt)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8X8_UNORM;
        default:                                return fmt;
        }
    }
};

std::shared_ptr<Dx12Core> RS::Dx12Core::Get()
{
	static std::shared_ptr<Dx12Core> s_Dx12Core = std::make_shared<Dx12Core>();
	return s_Dx12Core;
}

void RS::Dx12Core::Init(DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat, uint32 backBufferCount, D3D_FEATURE_LEVEL minFeatureLevel, uint32 flags, UINT adapterIDoverride)
{
    m_BackBufferFormat = backBufferFormat;
    m_DepthBufferFormat = depthBufferFormat;
    m_BackBufferCount = backBufferCount;
    m_D3DMinFeatureLevel = minFeatureLevel;
    m_Options = flags;

    if (backBufferCount > MAX_BACK_BUFFER_COUNT)
    {
        throw std::out_of_range("backBufferCount too large");
    }

    if (minFeatureLevel < D3D_FEATURE_LEVEL_11_0)
    {
        throw std::out_of_range("minFeatureLevel too low");
    }
    if (m_Options & c_RequireTearingSupport)
    {
        m_Options |= c_AllowTearing;
    }
}

//void Dx12Core::Init(uint32 width, uint32 height, HWND hwnd)
//{
//    m_frameIndex = 0;
//    m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
//    m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));
//    //m_fenceValues{},
//    m_rtvDescriptorSize = 0;
//    
//    // ------------------------------------------------------------------------------------------------------------------
//    
//    ComPtr<IDXGIFactory4> factory = CreateFactory();
//    CreateDevice(factory, D3D_FEATURE_LEVEL_11_0);
//    
//    // ------------------------------------------------------------------------------------------------------------------
//    
//    // Describe and create the command queue.
//    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
//    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
//    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
//    
//    ThrowIfFailed(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueueDirect)));
//    
//    ComPtr<IDXGISwapChain1> swapChain;
//    DX12Factory::CreateSwapChain(factory, m_CommandQueueDirect, hwnd, FrameCount, width, height, swapChain);
//    
//    // This sample does not support fullscreen transitions.
//    ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
//    
//    ThrowIfFailed(swapChain.As(&m_SwapChain));
//    m_frameIndex = m_SwapChain->GetCurrentBackBufferIndex();
//    
//    // Create descriptor heaps.
//    {
//        // Describe and create a render target view (RTV) descriptor heap.
//        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
//        rtvHeapDesc.NumDescriptors  = FrameCount;
//        rtvHeapDesc.Type            = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
//        rtvHeapDesc.Flags           = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
//        ThrowIfFailed(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
//    
//        m_rtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
//    }
//    
//    // Create frame resources.
//    {
//        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
//    
//        // Create a RTV and a command allocator for each frame.
//        for (UINT n = 0; n < FrameCount; n++)
//        {
//            ThrowIfFailed(m_SwapChain->GetBuffer(n, IID_PPV_ARGS(&m_RenderTargets[n])));
//            m_Device->CreateRenderTargetView(m_RenderTargets[n].Get(), nullptr, rtvHandle);
//            rtvHandle.Offset(1, m_rtvDescriptorSize);
//    
//            ThrowIfFailed(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocators[n])));
//        }
//    }
//    
//    LoadAssets();
//}

void Dx12Core::Release()
{
    // Ensure that the GPU is no longer referencing resources that are about to be destroyed.
    WaitForGpu();
}

void RS::Dx12Core::InitializeDXGIAdapter()
{
    bool debugDXGI = false;

#ifdef RS_CONFIG_DEBUG
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugInterface;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
        {
            debugInterface->EnableDebugLayer();
        }
        else
        {
            LOG_WARNING("Direct3D Debug Device is not available.");
        }

        ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
        {
            debugDXGI = true;

            ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_DXGIFactory)), "Failed to create DXGIFactory with debug flags!");

            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
        }
        else
        {
            LOG_WARNING("Filed to get Direct3D debug info queue.");
        }
    }
#endif

    if (!debugDXGI)
    {
        ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_DXGIFactory)), "Failed to Create DXGIFactory!");
    }

    // Determines whether tearing support is available for fullscreen borderless windows.
    if (m_Options & (c_AllowTearing | c_RequireTearingSupport))
    {
        BOOL allowTearing = FALSE;

        ComPtr<IDXGIFactory5> factory5;
        HRESULT hr = m_DXGIFactory.As(&factory5);
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing)
        {
            LOG_WARNING("Variable refresh rate displays are not supported.");
            ThrowIfFalse((m_Options & c_RequireTearingSupport) == 0, "Sample must be run on an OS with tearing support.");
            m_Options &= ~c_AllowTearing;
        }
    }

    InitializeAdapter(&m_Adapter);
}

void RS::Dx12Core::CreateDeviceResources()
{
    // Create the DX12 API device object.
    ThrowIfFailed(D3D12CreateDevice(m_Adapter.Get(), m_D3DMinFeatureLevel, IID_PPV_ARGS(&m_D3DDevice)));

#ifdef RS_CONFIG_DEBUG
    {
        // Configure debug device (if active).
        ComPtr<ID3D12InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(m_D3DDevice.As(&d3dInfoQueue)))
        {
            d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

            D3D12_MESSAGE_ID hide[] =
            {
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
            };
            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    // Determine maximum supported feature level for this device
    static const D3D_FEATURE_LEVEL s_featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
    {
        _countof(s_featureLevels), s_featureLevels, D3D_FEATURE_LEVEL_11_0
    };

    HRESULT hr = m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
    if (SUCCEEDED(hr))
    {
        m_D3DFeatureLevel = featLevels.MaxSupportedFeatureLevel;
    }
    else
    {
        m_D3DFeatureLevel = m_D3DMinFeatureLevel;
    }

    // Create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_D3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

    // Create descriptor heaps for render target views and depth stencil views.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = m_BackBufferCount;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    ThrowIfFailed(m_D3DDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&m_RTVDescriptorHeap)));

    m_RTVDescriptorSize = m_D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    if (m_DepthBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
        dsvDescriptorHeapDesc.NumDescriptors = 1;
        dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

        ThrowIfFailed(m_D3DDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&m_DSVDescriptorHeap)));
    }

    // Create a command allocator for each back buffer that will be rendered to.
    for (UINT n = 0; n < m_BackBufferCount; n++)
    {
        ThrowIfFailed(m_D3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocators[n])));
    }

    // Create a command list for recording graphics commands.
    ThrowIfFailed(m_D3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_CommandList)));
    ThrowIfFailed(m_CommandList->Close());

    // Create a fence for tracking GPU execution progress.
    ThrowIfFailed(m_D3DDevice->CreateFence(m_FenceValues[m_BackBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
    m_FenceValues[m_BackBufferIndex]++;

    m_FenceEvent.Attach(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    ThrowIfFalse(m_FenceEvent.IsValid(), "CreateEvent failed");
}

void RS::Dx12Core::CreateWindowSizeDependentResources()
{
    if (!m_Window)
    {
        ThrowIfFailed(E_HANDLE, L"Call SetWindow with a valid Win32 window handle.\n");
    }

    // Wait until all previous GPU work is complete.
    WaitForGpu();

    // Release resources that are tied to the swap chain and update fence values.
    for (UINT n = 0; n < m_BackBufferCount; n++)
    {
        m_RenderTargets[n].Reset();
        m_FenceValues[n] = m_FenceValues[m_BackBufferIndex];
    }

    // Determine the render target size in pixels.
    UINT backBufferWidth = std::max(m_OutputSize.right - m_OutputSize.left, 1l);
    UINT backBufferHeight = std::max(m_OutputSize.bottom - m_OutputSize.top, 1l);
    DXGI_FORMAT backBufferFormat = NoSRGB(m_BackBufferFormat);

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_SwapChain)
    {
        // If the swap chain already exists, resize it.
        HRESULT hr = m_SwapChain->ResizeBuffers(
            m_BackBufferCount,
            backBufferWidth,
            backBufferHeight,
            backBufferFormat,
            (m_Options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0
        );

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef RS_CONFIG_DEBUG
            
            //fmt::format()
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n", (hr == DXGI_ERROR_DEVICE_REMOVED) ? m_D3DDevice->GetDeviceRemovedReason() : hr);
            OutputDebugStringA(buff);
#endif
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            HandleDeviceLost();

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            ThrowIfFailed(hr);
        }
    }
    else
    {
        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = m_BackBufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = (m_Options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = { 0 };
        fsSwapChainDesc.Windowed = TRUE;

        // Create a swap chain for the window.
        ComPtr<IDXGISwapChain1> swapChain;

        // DXGI does not allow creating a swapchain targeting a window which has fullscreen styles(no border + topmost).
        // Temporarily remove the topmost property for creating the swapchain.
        // TODO: Implement this!!!!
        //bool prevIsFullscreen = Win32Application::IsFullscreen();
        //if (prevIsFullscreen)
        //{
        //    Win32Application::SetWindowZorderToTopMost(false);
        //}

        ThrowIfFailed(m_DXGIFactory->CreateSwapChainForHwnd(m_CommandQueue.Get(), m_Window, &swapChainDesc, &fsSwapChainDesc, nullptr, &swapChain));

        //if (prevIsFullscreen)
        //{
        //    Win32Application::SetWindowZorderToTopMost(true);
        //}

        ThrowIfFailed(swapChain.As(&m_SwapChain));

        // With tearing support enabled we will handle ALT+Enter key presses in the
        // window message loop rather than let DXGI handle it by calling SetFullscreenState.
        if (IsTearingSupported())
        {
            m_DXGIFactory->MakeWindowAssociation(m_Window, DXGI_MWA_NO_ALT_ENTER);
        }
    }

    // Obtain the back buffers for this window which will be the final render targets
    // and create render target views for each of them.
    for (UINT n = 0; n < m_BackBufferCount; n++)
    {
        ThrowIfFailed(m_SwapChain->GetBuffer(n, IID_PPV_ARGS(&m_RenderTargets[n])));

        wchar_t name[25] = {};
        swprintf_s(name, L"Render target %u", n);
        m_RenderTargets[n]->SetName(name);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = m_BackBufferFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), n, m_RTVDescriptorSize);
        m_D3DDevice->CreateRenderTargetView(m_RenderTargets[n].Get(), &rtvDesc, rtvDescriptor);
    }

    // Reset the index to the current back buffer.
    m_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

    if (m_DepthBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        // Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
        // on this surface.
        CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            m_DepthBufferFormat,
            backBufferWidth,
            backBufferHeight,
            1, // This depth stencil view has only one texture.
            1  // Use a single mipmap level.
        );
        depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = m_DepthBufferFormat;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;
        
        ThrowIfFailed(m_D3DDevice->CreateCommittedResource(&depthHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&m_DepthStencil)
        ));

        m_DepthStencil->SetName(L"Depth stencil");

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = m_DepthBufferFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        m_D3DDevice->CreateDepthStencilView(m_DepthStencil.Get(), &dsvDesc, m_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // Set the 3D rendering viewport and scissor rectangle to target the entire window.
    m_ScreenViewport.TopLeftX = m_ScreenViewport.TopLeftY = 0.f;
    m_ScreenViewport.Width = static_cast<float>(backBufferWidth);
    m_ScreenViewport.Height = static_cast<float>(backBufferHeight);
    m_ScreenViewport.MinDepth = D3D12_MIN_DEPTH;
    m_ScreenViewport.MaxDepth = D3D12_MAX_DEPTH;

    m_ScissorRect.left = m_ScissorRect.top = 0;
    m_ScissorRect.right = backBufferWidth;
    m_ScissorRect.bottom = backBufferHeight;
}

void RS::Dx12Core::SetWindow(HWND window, int width, int height)
{
    m_Window = window;

    m_OutputSize.left = m_OutputSize.top = 0;
    m_OutputSize.right = width;
    m_OutputSize.bottom = height;

    m_IsWindowVisible = true;
}

bool RS::Dx12Core::WindowSizeChanged(int width, int height, bool minimized)
{
    m_IsWindowVisible = !minimized;

    if (minimized || width == 0 || height == 0)
    {
        return false;
    }

    RECT newRc;
    newRc.left = newRc.top = 0;
    newRc.right = width;
    newRc.bottom = height;
    if (newRc.left == m_OutputSize.left
        && newRc.top == m_OutputSize.top
        && newRc.right == m_OutputSize.right
        && newRc.bottom == m_OutputSize.bottom)
    {
        return false;
    }

    m_OutputSize = newRc;
    CreateWindowSizeDependentResources();
    return true;
}

void RS::Dx12Core::HandleDeviceLost()
{
    if (m_DeviceNotify)
    {
        m_DeviceNotify->OnDeviceLost();
    }

    for (UINT n = 0; n < m_BackBufferCount; n++)
    {
        m_CommandAllocators[n].Reset();
        m_RenderTargets[n].Reset();
    }

    m_DepthStencil.Reset();
    m_CommandQueue.Reset();
    m_CommandList.Reset();
    m_Fence.Reset();
    m_RTVDescriptorHeap.Reset();
    m_DSVDescriptorHeap.Reset();
    m_SwapChain.Reset();
    m_D3DDevice.Reset();
    m_DXGIFactory.Reset();
    m_Adapter.Reset();

#ifdef RS_CONFIG_DEBUG
    ReportLiveObjects();
#endif

    InitializeDXGIAdapter();
    CreateDeviceResources();
    CreateWindowSizeDependentResources();

    if (m_DeviceNotify)
    {
        m_DeviceNotify->OnDeviceRestored();
    }
}

void RS::Dx12Core::Prepare(D3D12_RESOURCE_STATES beforeState)
{
    // Reset command list and allocator.
    ThrowIfFailed(m_CommandAllocators[m_BackBufferIndex]->Reset());
    ThrowIfFailed(m_CommandList->Reset(m_CommandAllocators[m_BackBufferIndex].Get(), nullptr));

    if (beforeState != D3D12_RESOURCE_STATE_RENDER_TARGET)
    {
        // Transition the render target into the correct state to allow for drawing into it.
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_BackBufferIndex].Get(), beforeState, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_CommandList->ResourceBarrier(1, &barrier);
    }
}

void RS::Dx12Core::Present(D3D12_RESOURCE_STATES beforeState)
{
    if (beforeState != D3D12_RESOURCE_STATE_PRESENT)
    {
        // Transition the render target to the state that allows it to be presented to the display.
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_BackBufferIndex].Get(), beforeState, D3D12_RESOURCE_STATE_PRESENT);
        m_CommandList->ResourceBarrier(1, &barrier);
    }

    ExecuteCommandList();

    HRESULT hr;
    if (m_Options & c_AllowTearing)
    {
        // Recommended to always use tearing if supported when using a sync interval of 0.
        // Note this will fail if in true 'fullscreen' mode.
        hr = m_SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    }
    else
    {
        // The first argument instructs DXGI to block until VSync, putting the application
        // to sleep until the next VSync. This ensures we don't waste any cycles rendering
        // frames that will never be displayed to the screen.
        hr = m_SwapChain->Present(1, 0);
    }

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        LOG_DEBUG("Device Lost on Present: Reason code {:#010x}", (hr == DXGI_ERROR_DEVICE_REMOVED) ? m_D3DDevice->GetDeviceRemovedReason() : hr);
        HandleDeviceLost();
    }
    else
    {
        ThrowIfFailed(hr);

        MoveToNextFrame();
    }
}

void RS::Dx12Core::ExecuteCommandList()
{
    ThrowIfFailed(m_CommandList->Close());
    ID3D12CommandList* commandLists[] = { m_CommandList.Get() };
    m_CommandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);
}

void RS::Dx12Core::WaitForGpu() noexcept
{
    if (m_CommandQueue && m_Fence && m_FenceEvent.IsValid())
    {
        // Schedule a Signal command in the GPU queue.
        UINT64 fenceValue = m_FenceValues[m_BackBufferIndex];
        if (SUCCEEDED(m_CommandQueue->Signal(m_Fence.Get(), fenceValue)))
        {
            // Wait until the Signal has been processed.
            if (SUCCEEDED(m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent.Get())))
            {
                WaitForSingleObjectEx(m_FenceEvent.Get(), INFINITE, FALSE);

                // Increment the fence value for the current frame.
                m_FenceValues[m_BackBufferIndex]++;
            }
        }
    }
}

//void Dx12Core::WaitForGPU()
//{
//    // Schedule a Signal command in the queue.
//    ThrowIfFailed(m_CommandQueueDirect->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));
//
//    // Wait until the fence has been processed.
//    ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
//    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
//
//    // Increment the fence value for the current frame.
//    m_fenceValues[m_frameIndex]++;
//}
//
//void Dx12Core::MoveToNextFrame()
//{
//    // Schedule a Signal command in the queue.
//    const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
//    ThrowIfFailed(m_CommandQueueDirect->Signal(m_fence.Get(), currentFenceValue));
//
//    // Update the frame index.
//    m_frameIndex = m_SwapChain->GetCurrentBackBufferIndex();
//
//    // If the next frame is not ready to be rendered yet, wait until it is ready.
//    if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
//    {
//        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
//        WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
//    }
//
//    // Set the fence value for the next frame.
//    m_fenceValues[m_frameIndex] = currentFenceValue + 1;
//}
//
//void Dx12Core::LoadAssets()
//{
//    // Create an empty root signature.
//    {
//        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
//        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//
//        ComPtr<ID3DBlob> signature;
//        ComPtr<ID3DBlob> error;
//        ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
//        ThrowIfFailed(m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
//    }
//
//    // Create the pipeline state, which includes compiling and loading shaders.
//    {
//        ComPtr<ID3DBlob> vertexShader;
//        ComPtr<ID3DBlob> pixelShader;
//
//#if defined(_DEBUG)
//        // Enable better shader debugging with the graphics debugging tools.
//        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
//#else
//        UINT compileFlags = 0;
//#endif
//
//        std::wstring p = Utils::Str2WStr(RS_SHADER_PATH + std::string("tmpShaders.hlsl"));
//        LPCWSTR pathW = p.c_str();
//        ThrowIfFailed(D3DCompileFromFile(pathW, nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
//        ThrowIfFailed(D3DCompileFromFile(pathW, nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));
//
//        // Define the vertex input layout.
//        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
//        {
//            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
//        };
//
//        // Describe and create the graphics pipeline state object (PSO).
//        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
//        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
//        psoDesc.pRootSignature = m_rootSignature.Get();
//        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
//        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
//        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//        psoDesc.DepthStencilState.DepthEnable = FALSE;
//        psoDesc.DepthStencilState.StencilEnable = FALSE;
//        psoDesc.SampleMask = UINT_MAX;
//        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//        psoDesc.NumRenderTargets = 1;
//        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
//        psoDesc.SampleDesc.Count = 1;
//        ThrowIfFailed(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
//    }
//
//    // Create the command list.
//    ThrowIfFailed(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocators[m_frameIndex].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
//
//    // Command lists are created in the recording state, but there is nothing
//    // to record yet. The main loop expects it to be closed, so close it now.
//    ThrowIfFailed(m_commandList->Close());
//
//    // Create the vertex buffer.
//    {
//        // Define the geometry for a triangle.
//        float aspectRatio = m_viewport.Width / m_viewport.Height;
//        Vertex triangleVertices[] =
//        {
//            { { 0.0f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
//            { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
//            { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
//        };
//
//        const UINT vertexBufferSize = sizeof(triangleVertices);
//
//        // Note: using upload heaps to transfer static data like vert buffers is not 
//        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
//        // over. Please read up on Default Heap usage. An upload heap is used here for 
//        // code simplicity and because there are very few verts to actually transfer.
//        ThrowIfFailed(m_Device->CreateCommittedResource(
//            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
//            D3D12_HEAP_FLAG_NONE,
//            &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
//            D3D12_RESOURCE_STATE_GENERIC_READ,
//            nullptr,
//            IID_PPV_ARGS(&m_vertexBuffer)));
//
//        // Copy the triangle data to the vertex buffer.
//        UINT8* pVertexDataBegin;
//        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
//        ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
//        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
//        m_vertexBuffer->Unmap(0, nullptr);
//
//        // Initialize the vertex buffer view.
//        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
//        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
//        m_vertexBufferView.SizeInBytes = vertexBufferSize;
//    }
//
//    // Create synchronization objects and wait until assets have been uploaded to the GPU.
//    {
//        ThrowIfFailed(m_Device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
//        m_fenceValues[m_frameIndex]++;
//
//        // Create an event handle to use for frame synchronization.
//        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
//        if (m_fenceEvent == nullptr)
//        {
//            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
//        }
//
//        // Wait for the command list to execute; we are reusing the same command 
//        // list in our main loop but for now, we just want to wait for setup to 
//        // complete before continuing.
//        WaitForGPU();
//    }
//}
//
//void Dx12Core::Render()
//{
//    // Record all the commands we need to render the scene into the command list.
//    PopulateCommandList();
//
//    // Execute the command list.
//    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
//    m_CommandQueueDirect->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
//
//    // Present the frame.
//    ThrowIfFailed(m_SwapChain->Present(1, 0));
//
//    MoveToNextFrame();
//}
//
//void Dx12Core::PopulateCommandList()
//{
//    // Command list allocators can only be reset when the associated 
//    // command lists have finished execution on the GPU; apps should use 
//    // fences to determine GPU execution progress.
//    ThrowIfFailed(m_CommandAllocators[m_frameIndex]->Reset());
//
//    // However, when ExecuteCommandList() is called on a particular command 
//    // list, that command list can then be reset at any time and must be before 
//    // re-recording.
//    ThrowIfFailed(m_commandList->Reset(m_CommandAllocators[m_frameIndex].Get(), m_pipelineState.Get()));
//
//    // Set necessary state.
//    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
//    m_commandList->RSSetViewports(1, &m_viewport);
//    m_commandList->RSSetScissorRects(1, &m_scissorRect);
//
//    // Indicate that the back buffer will be used as a render target.
//    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
//    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
//
//    // Record commands.
//    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
//    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
//    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
//    m_commandList->DrawInstanced(3, 1, 0, 0);
//
//    // Indicate that the back buffer will now be used to present.
//    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
//
//    ThrowIfFailed(m_commandList->Close());
//}
//
//ComPtr<IDXGIFactory4> RS::Dx12Core::CreateFactory()
//{
//    UINT dxgiFactoryFlags = 0;
//
//#if defined(RS_CONFIG_DEBUG)
//    // Enable the debug layer (requires the Graphics Tools "optional feature").
//    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
//    ComPtr<ID3D12Debug> debugController;
//    {
//        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
//        {
//            debugController->EnableDebugLayer();
//
//            // Enable additional debug layers.
//            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
//        }
//    }
//#endif
//
//    ComPtr<IDXGIFactory4> factory;
//    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
//    return factory;
//}
//
//void RS::Dx12Core::CreateDevice(ComPtr<IDXGIFactory4>& factory, D3D_FEATURE_LEVEL featureLevel)
//{
//    ComPtr<IDXGIAdapter1> hardwareAdapter;
//    DX12Factory::GetHardwareAdapter(factory.Get(), &hardwareAdapter);
//    ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), featureLevel, IID_PPV_ARGS(&m_Device)));
//}

// Prepare to render the next frame.
void Dx12Core::MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = m_FenceValues[m_BackBufferIndex];
    ThrowIfFailed(m_CommandQueue->Signal(m_Fence.Get(), currentFenceValue));

    // Update the back buffer index.
    m_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_Fence->GetCompletedValue() < m_FenceValues[m_BackBufferIndex])
    {
        ThrowIfFailed(m_Fence->SetEventOnCompletion(m_FenceValues[m_BackBufferIndex], m_FenceEvent.Get()));
        WaitForSingleObjectEx(m_FenceEvent.Get(), INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    m_FenceValues[m_BackBufferIndex] = currentFenceValue + 1;
}

void Dx12Core::InitializeAdapter(IDXGIAdapter1** ppAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIFactory6> factory6;
    HRESULT hr = m_DXGIFactory.As(&factory6);
    ThrowIfFailed(hr, "DXGI 1.6 not supported!");
    
    for (UINT adapterID = 0; DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(adapterID, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)); ++adapterID)
    {
        if (m_AdapterIDoverride != UINT_MAX && adapterID != m_AdapterIDoverride)
        {
            continue;
        }

        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc));

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), m_D3DMinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
        {
            m_AdapterID = adapterID;
            m_AdapterDescription = desc.Description;

            LOG_INFO("Direct3D Adapter({}) : VID: {:04X}, PID : {:04X} - {}", adapterID, desc.VendorId, desc.DeviceId, Utils::ToString(desc.Description).c_str());

            break;
        }
    }

#if !defined(NDEBUG)
    if (!adapter && m_AdapterIDoverride == UINT_MAX)
    {
        // Try WARP12 instead
        HRESULT hr = m_DXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
        ThrowIfFailed(hr, "WARP12 not available. Enable the 'Graphics Tools' optional feature");

        LOG_DEBUG("Direct3D Adapter - WARP12");
    }
#endif

    if (!adapter)
    {
        if (m_AdapterIDoverride != UINT_MAX)
        {
            throw std::exception("Unavailable adapter requested.");
        }
        else
        {
            throw std::exception("Unavailable adapter.");
        }
    }

    *ppAdapter = adapter.Detach();
}

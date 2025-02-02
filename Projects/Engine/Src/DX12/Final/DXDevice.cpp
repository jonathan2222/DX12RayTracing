#include "PreCompiled.h"
#include "DXDevice.h"
#include "Utils/Utils.h"

#ifdef RS_CONFIG_DEBUG
#include <dxgidebug.h>
#endif

void RS::DX12::DXDevice::Init(D3D_FEATURE_LEVEL minFeatureLevel, DXGIFlags dxgiFlags)
{
    m_D3DMinFeatureLevel = minFeatureLevel;
    m_DxgiFlags = dxgiFlags;

    CreateFactory();
    FetchDX12Adapter(&m_pAdapter);
    CreateDevice();

#ifdef RS_CONFIG_DEBUG
    ID3D12InfoQueue* pInfoQueue = nullptr;
    if (SUCCEEDED(m_pDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue))))
    {
        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] =
        {
            // This occurs when there are uninitialized descriptors in a descriptor table, even when a
            // shader does not access the missing descriptors.  I find this is common when switching
            // shader permutations and not wanting to change much code to reorder resources.
            D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,

            // Triggered when a shader does not export all color components of a render target, such as
            // when only writing RGB to an R10G10B10A2 buffer, ignoring alpha.
            D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,

            // This occurs when a descriptor table is unbound even when a shader does not access the missing
            // descriptors.  This is common with a root signature shared between disparate shaders that
            // don't all need the same types of resources.
            D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,

            // RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS,

            // Suppress errors from calling ResolveQueryData with timestamps that weren't requested on a given frame.
            D3D12_MESSAGE_ID_RESOLVE_QUERY_INVALID_QUERY_STATE,

            // Ignoring InitialState D3D12_RESOURCE_STATE_COPY_DEST. Buffers are effectively created in state D3D12_RESOURCE_STATE_COMMON.
            //D3D12_MESSAGE_ID_CREATERESOURCE_STATE_IGNORED,
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        //NewFilter.DenyList.NumCategories = _countof(Categories);
        //NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;

        pInfoQueue->PushStorageFilter(&NewFilter);
        pInfoQueue->Release();
    }
#endif
}

void RS::DX12::DXDevice::Release()
{
    DX12_RELEASE(m_pFactory);
    DX12_RELEASE(m_pAdapter);
    DX12_RELEASE(m_pDevice);
}

void RS::DX12::DXDevice::CreateFactory()
{
    bool debugDXGI = false;

#ifdef RS_CONFIG_DEBUG
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugInterface;
        ComPtr<ID3D12Debug1> debugInterface1;
        //ComPtr<ID3D12Debug5> debugInterface5;
        //ComPtr<ID3D12DebugDevice2> debugInterfaceDevice2;
        //ComPtr<ID3D12DebugCommandList2> debugInterfaceC2;
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

            DXCall(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_pFactory)));

            DXCallVerbose(dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true));
            DXCallVerbose(dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true));
        }
        else
        {
            LOG_WARNING("Filed to get Direct3D debug info queue.");
        }
    }
#endif

    if (!debugDXGI)
    {
        DXCall(CreateDXGIFactory1(IID_PPV_ARGS(&m_pFactory)));
    }

    // If requested, determines whether tearing is supported.
    if (m_DxgiFlags & (DXGIFlag::ALLOW_TEARING | DXGIFlag::REQUIRE_TEARING_SUPPORT))
    {
        BOOL allowTearing = FALSE;

        ComPtr<IDXGIFactory5> factory5;
        HRESULT hr = m_pFactory->QueryInterface(IID_PPV_ARGS(&factory5));
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing)
        {
            LOG_WARNING("Variable refresh rate (tearing) displays are not supported.");
            RS_ASSERT((m_DxgiFlags & DXGIFlag::REQUIRE_TEARING_SUPPORT) == 0, "Tearing is not supported in this device!");
            m_DxgiFlags &= ~DXGIFlag::ALLOW_TEARING;
        }
    }
}

void RS::DX12::DXDevice::FetchDX12Adapter(IDXGIAdapter1** ppAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIFactory6> factory6;
    {
        DXCall(m_pFactory->QueryInterface(IID_PPV_ARGS(&factory6)));
    }

    for (uint32 adapterID = 0; DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference((UINT)adapterID, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)); ++adapterID)
    {
        DXGI_ADAPTER_DESC1 desc;
        DXCall(adapter->GetDesc1(&desc));

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), m_D3DMinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
        {
            m_AdapterID = adapterID;

            LOG_INFO("Direct3D Adapter({}) : VID: {:04X}, PID : {:04X} - {}", adapterID, desc.VendorId, desc.DeviceId, Utils::ToString(desc.Description).c_str());

            break;
        }
    }

#if !defined(NDEBUG)
    if (!adapter)
    {
        // Try WARP12 instead
        DXCall(m_pFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));

        LOG_DEBUG("Direct3D Adapter - WARP12");
    }
#endif

    if (!adapter)
    {
        throw std::exception("Unavailable adapter requested.");
    }

    *ppAdapter = adapter.Detach();
}

void RS::DX12::DXDevice::CreateDevice()
{
    // Create the DX12 API device object.
    DXCall(D3D12CreateDevice(m_pAdapter, m_D3DMinFeatureLevel, IID_PPV_ARGS(&m_pDevice)));
    DX12_SET_DEBUG_NAME(m_pDevice, "D3D12 Device");

#ifdef RS_CONFIG_DEBUG
    {
        // Configure debug device (if active).
        ComPtr<ID3D12InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(m_pDevice->QueryInterface(IID_PPV_ARGS(&d3dInfoQueue))))
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

    auto FeatureLevelToStr = [](D3D_FEATURE_LEVEL level)
        {
            switch (level)
            {
            case D3D_FEATURE_LEVEL_1_0_CORE:    return "1.0 Core";
            case D3D_FEATURE_LEVEL_9_1:         return "9.1";
            case D3D_FEATURE_LEVEL_9_2:         return "9.2";
            case D3D_FEATURE_LEVEL_9_3:         return "9.3";
            case D3D_FEATURE_LEVEL_10_0:        return "10.0";
            case D3D_FEATURE_LEVEL_10_1:        return "10.1";
            case D3D_FEATURE_LEVEL_11_0:        return "11.0";
            case D3D_FEATURE_LEVEL_11_1:        return "11.1";
            case D3D_FEATURE_LEVEL_12_0:        return "12.0";
            case D3D_FEATURE_LEVEL_12_1:        return "12.1";
            case D3D_FEATURE_LEVEL_12_2:        return "12.2";
            default:
                return "Unknown";
                break;
            }
        };

    D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
    {
        _countof(s_featureLevels), s_featureLevels, D3D_FEATURE_LEVEL_11_0
    };

    HRESULT hr = m_pDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
    if (SUCCEEDED(hr))
    {
        m_D3DFeatureLevel = featLevels.MaxSupportedFeatureLevel;
    }
    else
    {
        m_D3DFeatureLevel = m_D3DMinFeatureLevel;
    }

    LOG_INFO("Direct3D Feature Level: {}", FeatureLevelToStr(m_D3DFeatureLevel));
}

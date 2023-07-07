#include "PreCompiled.h"
#include "Dx12Device.h"
#include "Utils/Utils.h"

void RS::DX12::Dx12Device::Init(D3D_FEATURE_LEVEL minFeatureLevel, DXGIFlags dxgiFlags)
{
    m_D3DMinFeatureLevel = minFeatureLevel;
    m_DxgiFlags = dxgiFlags;

    CreateFactory();
    FetchDX12Adapter(&m_Adapter);
    CreateDevice();
}

void RS::DX12::Dx12Device::Release()
{
    DX12_RELEASE(m_Factory);
    DX12_RELEASE(m_Adapter);
    DX12_RELEASE(m_Device);
}

void RS::DX12::Dx12Device::CreateFactory()
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

            DXCall(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_Factory)));

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
        DXCall(CreateDXGIFactory1(IID_PPV_ARGS(&m_Factory)));
    }

    // If requested, determines whether tearing is supported.
    if (m_DxgiFlags & (DXGIFlag::ALLOW_TEARING | DXGIFlag::REQUIRE_TEARING_SUPPORT))
    {
        BOOL allowTearing = FALSE;

        ComPtr<IDXGIFactory5> factory5;
        HRESULT hr = m_Factory->QueryInterface(IID_PPV_ARGS(&factory5));
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing)
        {
            LOG_WARNING("Variable refresh rate (tearing) displays are not supported.");
            ThrowIfFalse((m_DxgiFlags & DXGIFlag::REQUIRE_TEARING_SUPPORT) == 0, "Tearing is not supported in this device!");
            m_DxgiFlags &= ~DXGIFlag::ALLOW_TEARING;
        }
    }
}

void RS::DX12::Dx12Device::FetchDX12Adapter(IDXGIAdapter1** ppAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIFactory6> factory6;
    {
        DXCall(m_Factory->QueryInterface(IID_PPV_ARGS(&factory6)));
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
        DXCall(m_Factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));

        LOG_DEBUG("Direct3D Adapter - WARP12");
    }
#endif

    if (!adapter)
    {
        throw std::exception("Unavailable adapter requested.");
    }

    *ppAdapter = adapter.Detach();
}

void RS::DX12::Dx12Device::CreateDevice()
{
    // Create the DX12 API device object.
    DXCall(D3D12CreateDevice(m_Adapter, m_D3DMinFeatureLevel, IID_PPV_ARGS(&m_Device)));
    DX12_SET_DEBUG_NAME(m_Device, "D3D12 Device");

#ifdef RS_CONFIG_DEBUG
    {
        // Configure debug device (if active).
        ComPtr<ID3D12InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&d3dInfoQueue))))
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

    HRESULT hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
    if (SUCCEEDED(hr))
    {
        m_D3DFeatureLevel = featLevels.MaxSupportedFeatureLevel;
    }
    else
    {
        m_D3DFeatureLevel = m_D3DMinFeatureLevel;
    }
}

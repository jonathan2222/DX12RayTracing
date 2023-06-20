#include "PreCompiled.h"
#include "Resource.h"

#include "DX12/NewCore/DX12Core3.h"

RS::Resource::~Resource()
{
    if (!m_WasFreed)
    {
        Free();
        m_WasFreed = true;
    }
}

RS::Resource::Resource(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* pClearValue)
    : m_WasFreed(false)
{
    if (pClearValue)
        m_pD3D12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*pClearValue);

    auto pDevice = DX12Core3::Get()->GetD3D12Device();
    DXCall(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON, m_pD3D12ClearValue.get(), IID_PPV_ARGS(&m_pD3D12Resource)));

    ResourceStateTracker::AddGlobalResourceState(m_pD3D12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

    if (LaunchArguments::Contains(LaunchParams::logResources))
    {
        m_ID = GenerateID();
        DX12Core3::AddToLifetimeTracker(m_ID);
    }

    CheckFeatureSupport();
}

RS::Resource::Resource(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, const D3D12_CLEAR_VALUE* pClearValue)
    : m_pD3D12Resource(pResource)
    , m_WasFreed(false)
{
    if (pClearValue)
        m_pD3D12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*pClearValue);

    if (LaunchArguments::Contains(LaunchParams::logResources))
    {
        m_ID = GenerateID();
        DX12Core3::AddToLifetimeTracker(m_ID);
    }

    CheckFeatureSupport();
}

//D3D12_CPU_DESCRIPTOR_HANDLE RS::Resource::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
//{
//    D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
//    auto pDevice = DX12Core3::Get()->GetD3D12Device();
//    pDevice->CreateShaderResourceView(m_pD3D12Resource.Get(), srvDesc, handle);
//    return handle;
//}
//
//D3D12_CPU_DESCRIPTOR_HANDLE RS::Resource::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
//{
//    D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
//    auto pDevice = DX12Core3::Get()->GetD3D12Device();
//    pDevice->CreateUnorderedAccessView(m_pD3D12Resource.Get(), nullptr, uavDesc, handle);
//    return handle;
//}

void RS::Resource::Free() const
{
    if (m_WasFreed)
    {
        LOG_WARNING("Trying to free a resource multiple times!");
        return;
    }

    DX12Core3::Get()->FreeResource(m_pD3D12Resource);
    m_WasFreed = true;

    if (LaunchArguments::Contains(LaunchParams::logResources))
        DX12Core3::RemoveFromLifetimeTracker(m_ID);
}

void RS::Resource::SetName(const std::string& name)
{
    DXCallVerbose(m_pD3D12Resource->SetName(Utils::ToWString(name).c_str()));
    m_Name = name;

    if (LaunchArguments::Contains(LaunchParams::logResources))
        DX12Core3::SetNameOfLifetimeTrackedResource(m_ID, name);
}

std::string RS::Resource::GetName() const
{
    return m_Name;
}

D3D12_RESOURCE_DESC RS::Resource::GetD3D12ResourceDesc() const
{
    D3D12_RESOURCE_DESC desc = {};
    if (m_pD3D12Resource)
        desc = m_pD3D12Resource->GetDesc();

    return desc;
}

bool RS::Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const
{
    return (m_FormatSupport.Support1 & formatSupport) != 0;
}

bool RS::Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const
{
    return (m_FormatSupport.Support2 & formatSupport) != 0;
}

bool RS::Resource::IsValid() const
{
    return m_pD3D12Resource != nullptr;
}

D3D12_RESOURCE_DESC RS::Resource::CheckFeatureSupport()
{
    auto pDevice = DX12Core3::Get()->GetD3D12Device();

    D3D12_RESOURCE_DESC desc = m_pD3D12Resource->GetDesc();
    m_FormatSupport.Format = desc.Format;
    DXCallVerbose(pDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &m_FormatSupport, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
    
    return desc;
}

RS::DescriptorAllocation RS::Resource::CreateShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
    auto pCore = DX12Core3::Get();
    auto pDevice = pCore->GetD3D12Device();
    auto srv = pCore->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    pDevice->CreateShaderResourceView(m_pD3D12Resource.Get(), srvDesc, srv.GetDescriptorHandle());

    return srv;
}

RS::DescriptorAllocation RS::Resource::CreateUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
    auto pCore = DX12Core3::Get();
    auto pDevice = pCore->GetD3D12Device();
    auto uav = pCore->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    pDevice->CreateUnorderedAccessView(m_pD3D12Resource.Get(), nullptr, uavDesc, uav.GetDescriptorHandle());

    return uav;
}

uint64 RS::Resource::GenerateID() const
{
    // TODO: Bad generator, should use the ones that gets freed too. This might loop around to some id that is not free!
    std::lock_guard<std::mutex> lock(s_IDGeneratorMutex);
    return s_IDGenerator++;
}

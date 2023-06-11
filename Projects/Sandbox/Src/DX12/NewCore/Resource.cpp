#include "PreCompiled.h"
#include "Resource.h"

#include "DX12/NewCore/DX12Core3.h"

RS::Resource::Resource()
    : m_pD3D12Resource(nullptr)
    , m_WasFreed(false)
{
}

RS::Resource::~Resource()
{
    if (!m_WasFreed)
    {
        Free();
        m_WasFreed = true;
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE RS::Resource::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
    auto pDevice = DX12Core3::Get()->GetD3D12Device();
    pDevice->CreateShaderResourceView(m_pD3D12Resource.Get(), srvDesc, handle);
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE RS::Resource::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
    auto pDevice = DX12Core3::Get()->GetD3D12Device();
    pDevice->CreateUnorderedAccessView(m_pD3D12Resource.Get(), nullptr, uavDesc, handle);
    return handle;
}

void RS::Resource::Free() const
{
    RS_ASSERT(m_WasFreed, "Trying to free a resource multiple times!");
    DX12Core3::Get()->FreeResource(m_pD3D12Resource);
    m_WasFreed = true;
}

void RS::Resource::SetName(const std::string& name)
{
    DXCallVerbose(m_pD3D12Resource->SetName(Utils::ToWString(name).c_str()));
    m_Name = name;
}

std::string RS::Resource::GetName() const
{
    return m_Name;
}

void RS::Resource::SetDescriptor(DescriptorAllocation&& descriptorAllocation)
{
    m_DescriptorAllocation = std::move(descriptorAllocation);
}

D3D12_RESOURCE_DESC RS::Resource::GetD3D12ResourceDesc() const
{
    D3D12_RESOURCE_DESC desc = {};
    if (m_pD3D12Resource)
        desc = m_pD3D12Resource->GetDesc();

    return desc;
}

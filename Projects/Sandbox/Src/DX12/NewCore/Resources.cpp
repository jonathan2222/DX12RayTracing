#include "PreCompiled.h"
#include "Resources.h"

#include "DX12/NewCore/DX12Core3.h"

namespace Internal
{
    // Get a UAV description that matches the resource description.
    D3D12_UNORDERED_ACCESS_VIEW_DESC GetUAVDesc(const D3D12_RESOURCE_DESC& resDesc, UINT mipSlice, UINT arraySlice = 0, UINT planeSlice = 0)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = resDesc.Format;

        switch (resDesc.Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if (resDesc.DepthOrArraySize > 1)
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                uavDesc.Texture1DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
                uavDesc.Texture1DArray.FirstArraySlice = arraySlice;
                uavDesc.Texture1DArray.MipSlice = mipSlice;
            }
            else
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                uavDesc.Texture1D.MipSlice = mipSlice;
            }
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (resDesc.DepthOrArraySize > 1)
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                uavDesc.Texture2DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
                uavDesc.Texture2DArray.FirstArraySlice = arraySlice;
                uavDesc.Texture2DArray.PlaneSlice = planeSlice;
                uavDesc.Texture2DArray.MipSlice = mipSlice;
            }
            else
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                uavDesc.Texture2D.PlaneSlice = planeSlice;
                uavDesc.Texture2D.MipSlice = mipSlice;
            }
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            uavDesc.Texture3D.WSize = resDesc.DepthOrArraySize - arraySlice;
            uavDesc.Texture3D.FirstWSlice = arraySlice;
            uavDesc.Texture3D.MipSlice = mipSlice;
            break;
        default:
            throw std::exception("Invalid resource dimension.");
        }

        return uavDesc;
    }
}

RS::Buffer::Buffer(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* pClearValue, const std::string& name)
	: Resource(resourceDesc, pClearValue)
{
	DX12_SET_DEBUG_NAME(m_pD3D12Resource.Get(), name);
    CreateViews();
}

RS::Buffer::Buffer(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, const std::string& name)
	: Resource(pResource)
{
	DX12_SET_DEBUG_NAME(m_pD3D12Resource.Get(), name);
    CreateViews();
}

RS::Buffer::~Buffer()
{
}

void RS::Buffer::CreateViews()
{
    std::lock_guard<std::mutex> lock(m_ShaderResourceViewsMutex);
    std::lock_guard<std::mutex> guard(m_UnorderedAccessViewsMutex);

    // SRVs and UAVs will be created as needed.
    m_ShaderResourceViews.clear();
    m_UnorderedAccessViews.clear();
}

D3D12_CPU_DESCRIPTOR_HANDLE RS::Buffer::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
    std::size_t hash = 0;
    if (srvDesc)
    {
        hash = std::hash<D3D12_SHADER_RESOURCE_VIEW_DESC>{}(*srvDesc);
    }

    std::lock_guard<std::mutex> lock(m_ShaderResourceViewsMutex);

    auto iter = m_ShaderResourceViews.find(hash);
    if (iter == m_ShaderResourceViews.end())
    {
        auto srv = CreateShaderResourceView(srvDesc);
        iter = m_ShaderResourceViews.insert({ hash, std::move(srv) }).first;
    }

    return iter->second.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE RS::Buffer::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
    std::size_t hash = 0;
    if (uavDesc)
    {
        hash = std::hash<D3D12_UNORDERED_ACCESS_VIEW_DESC>{}(*uavDesc);
    }

    std::lock_guard<std::mutex> guard(m_UnorderedAccessViewsMutex);

    auto iter = m_UnorderedAccessViews.find(hash);
    if (iter == m_UnorderedAccessViews.end())
    {
        auto uav = CreateUnorderedAccessView(uavDesc);
        iter = m_UnorderedAccessViews.insert({ hash, std::move(uav) }).first;
    }

    return iter->second.GetDescriptorHandle();
}

RS::VertexBuffer::VertexBuffer(uint32 stride, const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* pClearValue, const std::string& name)
    : Buffer(resourceDesc, pClearValue, name)
    , m_StrideInBytes(stride)
    , m_SizeInBytes(resourceDesc.Width)
{
    
}

RS::VertexBuffer::VertexBuffer(uint32 stride, Microsoft::WRL::ComPtr<ID3D12Resource> pResource, const std::string& name)
    : Buffer(pResource, name)
    , m_StrideInBytes(stride)
{
    m_SizeInBytes = GetD3D12ResourceDesc().Width;
}

RS::VertexBuffer::~VertexBuffer()
{
}

D3D12_VERTEX_BUFFER_VIEW RS::VertexBuffer::CreateView() const
{
    D3D12_VERTEX_BUFFER_VIEW view = {};
    view.BufferLocation = m_pD3D12Resource->GetGPUVirtualAddress();
    view.SizeInBytes = m_SizeInBytes;
    view.StrideInBytes = m_StrideInBytes;
    return view;
}

RS::Texture::Texture(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* pClearValue, const std::string& name)
	: Resource(resourceDesc, pClearValue)
{
	DX12_SET_DEBUG_NAME(m_pD3D12Resource.Get(), name);
    CreateViews();
}

RS::Texture::Texture(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, const std::string& name)
	: Resource(pResource)
{
	DX12_SET_DEBUG_NAME(m_pD3D12Resource.Get(), name);
    CreateViews();
}

void RS::Texture::Resize(uint32 width, uint32 height, uint32 depthOrArraySize)
{
    if (m_pD3D12Resource)
    {
        ResourceStateTracker::RemoveGlobalResourceState(m_pD3D12Resource.Get());

        CD3DX12_RESOURCE_DESC resourceDesc(m_pD3D12Resource->GetDesc());
        resourceDesc.Width = std::max(1u, width);
        resourceDesc.Height = std::max(1u, height);
        resourceDesc.DepthOrArraySize = depthOrArraySize;

        auto pDevice = DX12Core3::Get()->GetD3D12Device();
        DXCall(pDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            m_pD3D12ClearValue.get(),
            IID_PPV_ARGS(&m_pD3D12Resource)
        ));

        std::wstring name = Utils::ToWString(m_Name);
        m_pD3D12Resource->SetName(name.c_str());

        ResourceStateTracker::AddGlobalResourceState(m_pD3D12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

        CreateViews();
    }
}

void RS::Texture::CreateViews()
{
    if (m_pD3D12Resource)
    {
        auto pCore = DX12Core3::Get();
        auto pDevice = pCore->GetD3D12Device();

        CD3DX12_RESOURCE_DESC desc(CheckFeatureSupport());

        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0 &&
            CheckRTVSupport(m_FormatSupport.Support1))
        {
            m_RenderTargetViewAllocation = pCore->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            pDevice->CreateRenderTargetView(m_pD3D12Resource.Get(), nullptr, m_RenderTargetViewAllocation.GetDescriptorHandle());
        }
        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0 &&
            CheckDSVSupport(m_FormatSupport.Support1))
        {
            m_DepthStencilAllocation = pCore->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            pDevice->CreateDepthStencilView(m_pD3D12Resource.Get(), nullptr, m_DepthStencilAllocation.GetDescriptorHandle());
        }
    }

    std::lock_guard<std::mutex> lock(m_ShaderResourceViewsMutex);
    std::lock_guard<std::mutex> guard(m_UnorderedAccessViewsMutex);

    // SRVs and UAVs will be created as needed.
    m_ShaderResourceViews.clear();
    m_UnorderedAccessViews.clear();
}

D3D12_CPU_DESCRIPTOR_HANDLE RS::Texture::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
    std::size_t hash = 0;
    if (srvDesc)
    {
        hash = std::hash<D3D12_SHADER_RESOURCE_VIEW_DESC>{}(*srvDesc);
    }

    std::lock_guard<std::mutex> lock(m_ShaderResourceViewsMutex);

    auto iter = m_ShaderResourceViews.find(hash);
    if (iter == m_ShaderResourceViews.end())
    {
        auto srv = CreateShaderResourceView(srvDesc);
        iter = m_ShaderResourceViews.insert({ hash, std::move(srv) }).first;
    }

    return iter->second.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE RS::Texture::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
    std::size_t hash = 0;
    if (uavDesc)
    {
        hash = std::hash<D3D12_UNORDERED_ACCESS_VIEW_DESC>{}(*uavDesc);
    }

    std::lock_guard<std::mutex> guard(m_UnorderedAccessViewsMutex);

    auto iter = m_UnorderedAccessViews.find(hash);
    if (iter == m_UnorderedAccessViews.end())
    {
        auto uav = CreateUnorderedAccessView(uavDesc);
        iter = m_UnorderedAccessViews.insert({ hash, std::move(uav) }).first;
    }

    return iter->second.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE RS::Texture::GetRenderTargetView() const
{
    return m_RenderTargetViewAllocation.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE RS::Texture::GetDepthStencilView() const
{
    return m_DepthStencilAllocation.GetDescriptorHandle();
}

bool RS::Texture::IsUAVCompatibleFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        //    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SINT:
        return true;
    default:
        return false;
    }
}

bool RS::Texture::IsSRGBFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return true;
    default:
        return false;
    }
}

bool RS::Texture::IsBGRFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return true;
    default:
        return false;
    }

}

bool RS::Texture::IsDepthFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_D16_UNORM:
        return true;
    default:
        return false;
    }
}

DXGI_FORMAT RS::Texture::GetTypelessFormat(DXGI_FORMAT format)
{
    DXGI_FORMAT typelessFormat = format;

    switch (format)
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        typelessFormat = DXGI_FORMAT_R32G32B32A32_TYPELESS;
        break;
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        typelessFormat = DXGI_FORMAT_R32G32B32_TYPELESS;
        break;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
        typelessFormat = DXGI_FORMAT_R16G16B16A16_TYPELESS;
        break;
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
        typelessFormat = DXGI_FORMAT_R32G32_TYPELESS;
        break;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        typelessFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
        break;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
        typelessFormat = DXGI_FORMAT_R10G10B10A2_TYPELESS;
        break;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
        typelessFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
        break;
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
        typelessFormat = DXGI_FORMAT_R16G16_TYPELESS;
        break;
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
        typelessFormat = DXGI_FORMAT_R32_TYPELESS;
        break;
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
        typelessFormat = DXGI_FORMAT_R8G8_TYPELESS;
        break;
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
        typelessFormat = DXGI_FORMAT_R16_TYPELESS;
        break;
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
        typelessFormat = DXGI_FORMAT_R8_TYPELESS;
        break;
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_BC1_TYPELESS;
        break;
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_BC2_TYPELESS;
        break;
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_BC3_TYPELESS;
        break;
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        typelessFormat = DXGI_FORMAT_BC4_TYPELESS;
        break;
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
        typelessFormat = DXGI_FORMAT_BC5_TYPELESS;
        break;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
        break;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_B8G8R8X8_TYPELESS;
        break;
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
        typelessFormat = DXGI_FORMAT_BC6H_TYPELESS;
        break;
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        typelessFormat = DXGI_FORMAT_BC7_TYPELESS;
        break;
    }

    return typelessFormat;
}

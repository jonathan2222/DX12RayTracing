#include "PreCompiled.h"
#include "DXReadbackBuffer.h"

#include "DX12/Final/DXCore.h"

void RS::DX12::DXReadbackBuffer::Create(const std::wstring& name, uint32 NumElements, uint32 ElementSize)
{
    Destroy();

    m_ElementCount = NumElements;
    m_ElementSize = ElementSize;
    m_BufferSize = NumElements * ElementSize;
    m_UsageState = D3D12_RESOURCE_STATE_COPY_DEST;

    // Create a readback buffer large enough to hold all texel data
    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_READBACK;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    // Readback buffers must be 1-dimensional, i.e. "buffer" not "texture2d"
    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Width = m_BufferSize;
    ResourceDesc.Height = 1;
    ResourceDesc.DepthOrArraySize = 1;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.SampleDesc.Count = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    DXCall(DXCore::GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_pResource)));

    m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

#ifdef RS_CONFIG_DEVELOPMENT
    m_pResource->SetName(name.c_str());
#else
    RS_UNUSED(name);
#endif
}

void* RS::DX12::DXReadbackBuffer::Map(void)
{
    void* Memory;
    CD3DX12_RANGE range(0, m_BufferSize);
    m_pResource->Map(0, &range, &Memory);
    return Memory;
}

void RS::DX12::DXReadbackBuffer::Unmap(void)
{
    CD3DX12_RANGE range(0, 0);
    m_pResource->Unmap(0, &range);
}

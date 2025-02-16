#include "PreCompiled.h"
#include "DXGPUResource.h"

bool RS::DX12::DXGPUResource::Map(uint32 subresource, const D3D12_RANGE* pReadRange, void** ppData, DataAccess access)
{
    // Debug
    D3D12_HEAP_PROPERTIES heapProperties;
    D3D12_HEAP_FLAGS heapFlags;
    GetHeapProperties(heapProperties, heapFlags);
    // Upload for write, readback for read
    RS_ASSERT(!(access == Read && heapProperties.Type != D3D12_HEAP_TYPE_READBACK), "Can only read from a resource in a readback heap!");
    RS_ASSERT(!(access == Write && heapProperties.Type != D3D12_HEAP_TYPE_UPLOAD), "Can only write to a resource in a upload heap!");

    return m_pResource->Map(subresource, pReadRange, ppData) == S_OK;
}

bool RS::DX12::DXGPUResource::Map(uint32 subresource, void** ppData, DataAccess access)
{
    D3D12_RANGE range;
    memset(&range, 0, sizeof(D3D12_RANGE));
    return Map(subresource, &range, ppData, access);
}

void RS::DX12::DXGPUResource::Unmap(uint32 subresource, const D3D12_RANGE* pWrittenRange)
{
    m_pResource->Unmap(subresource, pWrittenRange);
}

void RS::DX12::DXGPUResource::Unmap(uint32 subresource)
{
    D3D12_RANGE range;
    memset(&range, 0, sizeof(D3D12_RANGE));
    Unmap(subresource, &range);
}

void RS::DX12::DXGPUResource::GetHeapProperties(D3D12_HEAP_PROPERTIES& heapPropertiesOut, D3D12_HEAP_FLAGS& heapFlagsOut)
{
    m_pResource->GetHeapProperties(&heapPropertiesOut, &heapFlagsOut);
}

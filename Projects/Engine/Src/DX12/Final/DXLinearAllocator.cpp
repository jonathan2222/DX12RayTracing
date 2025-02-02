#include "PreCompiled.h"
#include "DXLinearAllocator.h"

#include "DX12/Final/DXCore.h"
#include "Utils/Misc/BitUtils.h"

#include <mutex>

RS::DX12::DXLinearAllocatorType RS::DX12::DXLinearAllocatorPageManager::sm_AutoType = DXLinearAllocatorType::GPUExclusive;
RS::DX12::DXLinearAllocatorPageManager RS::DX12::DXLinearAllocator::sm_PageManager[2];

RS::DX12::DXLinearAllocatorPageManager::DXLinearAllocatorPageManager()
{
    m_AllocationType = sm_AutoType;
    sm_AutoType = (DXLinearAllocatorType)((uint)sm_AutoType + 1);
    RS_ASSERT(sm_AutoType <= DXLinearAllocatorType::Count);
}

RS::DX12::DXLinearAllocationPage* RS::DX12::DXLinearAllocatorPageManager::RequestPage(void)
{
    std::lock_guard<std::mutex> LockGuard(m_Mutex);

    while (!m_RetiredPages.empty() && DXCore::GetCommandListManager()->IsFenceComplete(m_RetiredPages.front().first))
    {
        m_AvailablePages.push(m_RetiredPages.front().second);
        m_RetiredPages.pop();
    }

    DXLinearAllocationPage* PagePtr = nullptr;

    if (!m_AvailablePages.empty())
    {
        PagePtr = m_AvailablePages.front();
        m_AvailablePages.pop();
    }
    else
    {
        PagePtr = CreateNewPage();
        m_PagePool.emplace_back(PagePtr);
    }

    return PagePtr;
}

RS::DX12::DXLinearAllocationPage* RS::DX12::DXLinearAllocatorPageManager::CreateNewPage(size_t PageSize)
{
    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC ResourceDesc;
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Alignment = 0;
    ResourceDesc.Height = 1;
    ResourceDesc.DepthOrArraySize = 1;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.SampleDesc.Count = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_RESOURCE_STATES DefaultUsage;

    if (m_AllocationType == DXLinearAllocatorType::GPUExclusive)
    {
        HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        ResourceDesc.Width = PageSize == 0 ? GPUAllocatorPageSize : PageSize;
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        DefaultUsage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    else
    {
        HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        ResourceDesc.Width = PageSize == 0 ? CPUAllocatorPageSize : PageSize;
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        DefaultUsage = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    ID3D12Resource* pBuffer;
    DXCall(DXCore::GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
        &ResourceDesc, DefaultUsage, nullptr, IID_PPV_ARGS(&pBuffer)));

    pBuffer->SetName(L"LinearAllocator Page");

    return new DXLinearAllocationPage(pBuffer, DefaultUsage);
}

void RS::DX12::DXLinearAllocatorPageManager::DiscardPages(uint64_t FenceValue, const std::vector<DXLinearAllocationPage*>& UsedPages)
{
    std::lock_guard<std::mutex> LockGuard(m_Mutex);
    for (auto iter = UsedPages.begin(); iter != UsedPages.end(); ++iter)
        m_RetiredPages.push(std::make_pair(FenceValue, *iter));
}

void RS::DX12::DXLinearAllocatorPageManager::FreeLargePages(uint64_t FenceValue, const std::vector<DXLinearAllocationPage*>& LargePages)
{
    std::lock_guard<std::mutex> LockGuard(m_Mutex);

    while (!m_DeletionQueue.empty() && DXCore::GetCommandListManager()->IsFenceComplete(m_DeletionQueue.front().first))
    {
        delete m_DeletionQueue.front().second;
        m_DeletionQueue.pop();
    }

    for (auto iter = LargePages.begin(); iter != LargePages.end(); ++iter)
    {
        (*iter)->Unmap();
        m_DeletionQueue.push(std::make_pair(FenceValue, *iter));
    }
}

RS::DX12::DXDynAlloc RS::DX12::DXLinearAllocator::Allocate(size_t SizeInBytes, size_t Alignment)
{
    const size_t AlignmentMask = Alignment - 1;

    // Assert that it's a power of two.
    RS_ASSERT((AlignmentMask & Alignment) == 0);

    // Align the allocation
    const size_t AlignedSize = Utils::AlignUpWithMask(SizeInBytes, AlignmentMask);

    if (AlignedSize > m_PageSize)
        return AllocateLargePage(AlignedSize);

    m_CurOffset = Utils::AlignUp(m_CurOffset, Alignment);

    if (m_CurOffset + AlignedSize > m_PageSize)
    {
        RS_ASSERT(m_CurPage != nullptr);
        m_RetiredPages.push_back(m_CurPage);
        m_CurPage = nullptr;
    }

    if (m_CurPage == nullptr)
    {
        m_CurPage = sm_PageManager[(uint)m_AllocationType].RequestPage();
        m_CurOffset = 0;
    }

    DXDynAlloc ret(*m_CurPage, m_CurOffset, AlignedSize);
    ret.DataPtr = (uint8_t*)m_CurPage->m_CpuVirtualAddress + m_CurOffset;
    ret.GpuAddress = m_CurPage->m_GpuVirtualAddress + m_CurOffset;

    m_CurOffset += AlignedSize;

    return ret;
}

void RS::DX12::DXLinearAllocator::CleanupUsedPages(uint64_t FenceID)
{
    if (m_CurPage != nullptr)
    {
        m_RetiredPages.push_back(m_CurPage);
        m_CurPage = nullptr;
        m_CurOffset = 0;
    }

    sm_PageManager[(uint)m_AllocationType].DiscardPages(FenceID, m_RetiredPages);
    m_RetiredPages.clear();

    sm_PageManager[(uint)m_AllocationType].FreeLargePages(FenceID, m_LargePageList);
    m_LargePageList.clear();
}

RS::DX12::DXDynAlloc RS::DX12::DXLinearAllocator::AllocateLargePage(size_t SizeInBytes)
{
    DXLinearAllocationPage* OneOff = sm_PageManager[(uint)m_AllocationType].CreateNewPage(SizeInBytes);
    m_LargePageList.push_back(OneOff);

    DXDynAlloc ret(*OneOff, 0, SizeInBytes);
    ret.DataPtr = OneOff->m_CpuVirtualAddress;
    ret.GpuAddress = OneOff->m_GpuVirtualAddress;

    return ret;
}

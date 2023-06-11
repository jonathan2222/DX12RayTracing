#include "PreCompiled.h"
#include "DescriptorAllocator.h"

#include "DescriptorAllocatorPage.h"

RS::DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptorsPerHeap)
    : m_HeapType(type)
    , m_NumDescriptorsPerHeap(numDescriptorsPerHeap)
{
}

RS::DescriptorAllocator::~DescriptorAllocator()
{
}

RS::DescriptorAllocation RS::DescriptorAllocator::Allocate(uint32 numDescriptors)
{
    std::lock_guard<std::mutex> lock(m_AllocationMutex);

    DescriptorAllocation allocation;

    for (auto it = m_AvaliableHeaps.begin(); it != m_AvaliableHeaps.end(); ++it)
    {
        auto allocatorPage = m_HeapPool[*it];
        allocation = allocatorPage->Allocate(numDescriptors);

        if (allocatorPage->NumFreeHandles() == 0)
        {
            it = m_AvaliableHeaps.erase(it);
        }

        // A valid allocation has been found.
        if (!allocation.IsNull())
        {
            break;
        }
    }

    // No available heap could satisfy the requested number of descriptors.
    if (allocation.IsNull())
    {
        m_NumDescriptorsPerHeap = std::max(m_NumDescriptorsPerHeap, numDescriptors);

        auto newPage = CreateAllocatorPage();

        allocation = newPage->Allocate(numDescriptors);
    }

    return allocation;
}

void RS::DescriptorAllocator::ReleaseStaleDescriptors(uint64 frameNumber)
{
    std::lock_guard<std::mutex> lock(m_AllocationMutex);

    for (size_t heapIndex = 0; heapIndex < m_HeapPool.size(); ++heapIndex)
    {
        auto page = m_HeapPool[heapIndex];

        page->ReleaseStaleDescriptors(frameNumber);

        if (page->NumFreeHandles() > 0)
        {
            m_AvaliableHeaps.insert(heapIndex);
        }
    }
}

std::shared_ptr<RS::DescriptorAllocatorPage> RS::DescriptorAllocator::CreateAllocatorPage()
{
    auto newPage = std::make_shared<DescriptorAllocatorPage>(m_HeapType, m_NumDescriptorsPerHeap);
    m_HeapPool.emplace_back(newPage);
    m_AvaliableHeaps.insert(m_HeapPool.size() - 1);
    return newPage;
}

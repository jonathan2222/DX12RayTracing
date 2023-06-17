#include "PreCompiled.h"
#include "DescriptorAllocatorPage.h"

#include "DX12/NewCore/DX12Core3.h"

RS::DescriptorAllocatorPage::DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptors)
    : m_HeapType(type)
    , m_NumDescriptorsInHeap(numDescriptors)
{
    auto device = DX12Core3::Get()->GetD3D12Device();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = m_HeapType;
    heapDesc.NumDescriptors = m_NumDescriptorsInHeap;

    DXCall(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_d3d12DescriptorHeap)));

    m_BaseDescriptor = m_d3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    m_DescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(m_HeapType);
    m_NumFreeHandles = m_NumDescriptorsInHeap;

    // Initialize the free lists
    AddNewBlock(0, m_NumFreeHandles);
}

D3D12_DESCRIPTOR_HEAP_TYPE RS::DescriptorAllocatorPage::GetHeapType() const
{
    return m_HeapType;
}

bool RS::DescriptorAllocatorPage::HasSpace(uint32 numDescriptors) const
{
    return m_FreeListBySize.lower_bound(numDescriptors) != m_FreeListBySize.end();
}

uint32 RS::DescriptorAllocatorPage::NumFreeHandles() const
{
    return m_NumFreeHandles;
}

RS::DescriptorAllocation RS::DescriptorAllocatorPage::Allocate(uint32 numDescriptors)
{
    std::lock_guard<std::mutex> lock(m_AllocationMutex);

    // If there are less than the requested number of descriptors left in the heap.
    // Return a NULL descriptor and try another heap.
    if (numDescriptors > m_NumFreeHandles)
        return DescriptorAllocation();

    // Get the first block that is large enough to satisfy the request.
    auto smallestBlockIt = m_FreeListBySize.lower_bound(numDescriptors);
    if (smallestBlockIt == m_FreeListBySize.end())
    {
        // There was no free block that could satisfy the request.
        return DescriptorAllocation();
    }

    // The size of the smallest block that satisfies the request.
    auto blockSize = smallestBlockIt->first;

    // The pointer to the same entry in the FreeListByOffset map.
    auto offsetIt = smallestBlockIt->second;

    // The offset in the descriptor heap.
    auto offset = offsetIt->first;

    // Remove the existing free block from the free list.
    m_FreeListBySize.erase(smallestBlockIt);
    m_FreeListByOffset.erase(offsetIt);

    // Compute the new free block that results from splitting this block.
    OffsetType newOffset = offset + numDescriptors;
    SizeType newSize = blockSize - numDescriptors;

    if (newSize > 0)
    {
        // If the allocation didn't exactly match the requested size, return the left-over to the free list.
        AddNewBlock(newOffset, newSize);
    }

    // Decrement free handles.
    m_NumFreeHandles -= numDescriptors;

    return DescriptorAllocation(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(m_BaseDescriptor, offset, m_DescriptorHandleIncrementSize),
        numDescriptors, m_DescriptorHandleIncrementSize, shared_from_this());
}

void RS::DescriptorAllocatorPage::Free(DescriptorAllocation&& descriptor, uint64 frameNumber)
{
    // Compute the offset of the descriptor within the descriptor heap.
    OffsetType offset = ComputeOffset(descriptor.GetDescriptorHandle());

    std::lock_guard<std::mutex> lock(m_AllocationMutex);

    // Don't add the block directly to the free list until the frame has completed.
    m_StaleDescriptors.emplace(offset, descriptor.GetNumHandles(), frameNumber);
}

void RS::DescriptorAllocatorPage::ReleaseStaleDescriptors(uint64 frameNumber)
{
    std::lock_guard<std::mutex> lock(m_AllocationMutex);

    while (!m_StaleDescriptors.empty() && m_StaleDescriptors.front().frameNumber <= frameNumber)
    {
        auto& staleDescriptor = m_StaleDescriptors.front();

        // The offset of the descriptor in the heap.
        OffsetType offset = staleDescriptor.offset;

        // The number of descriptors that were allocated
        SizeType numDescriptors = staleDescriptor.size;

        FreeBlock(offset, numDescriptors);
        m_StaleDescriptors.pop();
    }
}

uint32 RS::DescriptorAllocatorPage::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    return static_cast<uint32>(handle.ptr - m_BaseDescriptor.ptr) / m_DescriptorHandleIncrementSize;
}

void RS::DescriptorAllocatorPage::AddNewBlock(uint32 offset, uint32 numDescriptors)
{
    auto offsetIt = m_FreeListByOffset.emplace(offset, numDescriptors);
    auto sizeIt = m_FreeListBySize.emplace(numDescriptors, offsetIt.first);
    offsetIt.first->second.freeListBySizeIt = sizeIt;
}

void RS::DescriptorAllocatorPage::FreeBlock(uint32 offset, uint32 numDescriptors)
{
    // Find the first element whose offset is greater than the specified offset.
    // This is the block that should appear after the block that is being freed.
    auto nextBlockIt = m_FreeListByOffset.upper_bound(offset);

    // Find the block that appears before the block being freed.
    auto prevBlockIt = nextBlockIt;

    // If it's not the frist block in the list.
    if (prevBlockIt != m_FreeListByOffset.begin())
    {
        // Go to the previous block in the list.
        --prevBlockIt;
    }
    else
    {
        // Otherwise, just set it to the end of the list to indicate that no block comes before the one being freed.
        prevBlockIt = m_FreeListByOffset.end();
    }

    // Add the number of free handles back to the heap.
    // This needs to be done before merging any blocks since merging blocks modifies the numDescriptors variable.
    m_NumFreeHandles += numDescriptors;

    if (prevBlockIt != m_FreeListByOffset.end() && offset == prevBlockIt->first + prevBlockIt->second.size)
    {
        // The previous block is exactly behind the block that is to be freed.
        //
        // PrevBlock.Offset           Offset
        // |                          |
        // |<-----PrevBlock.Size----->|<------Size-------->|
        //
        // Increase the block size by the size of merging with the previous block.
        offset = prevBlockIt->first;
        numDescriptors += prevBlockIt->second.size;

        // Remove the previous block from the free list.
        m_FreeListBySize.erase(prevBlockIt->second.freeListBySizeIt);
        m_FreeListByOffset.erase(prevBlockIt);
    }

    if (nextBlockIt != m_FreeListByOffset.end() && offset + numDescriptors == nextBlockIt->first)
    {
        // The next block is exactly in front of the block that is to be freed.
        //
        // Offset               NextBlock.Offset
        // |                    |
        // |<------Size-------->|<-----NextBlock.Size----->|
        // Increase the block size by the size of merging with the next block.
        numDescriptors += nextBlockIt->second.size;

        // Remove the next block from the free list.
        m_FreeListBySize.erase(nextBlockIt->second.freeListBySizeIt);
        m_FreeListByOffset.erase(nextBlockIt);
    }

    // Add the freed block to the free list.
    AddNewBlock(offset, numDescriptors);
}

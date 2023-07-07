#pragma once

#include "DescriptorAllocation.h"

#include "DX12/Dx12Device.h"

#include <mutex>
#include <set>
#include <vector>
#include <memory> // std::shared_ptr

namespace RS
{
	class DescriptorAllocatorPage;

	// From https://www.3dgep.com/learning-directx-12-3/#Descriptor_Allocator
	class DescriptorAllocator
	{
	public:
		DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptorsPerHeap = 256);
		virtual ~DescriptorAllocator();

		/**
		 * Allocate a number of contiguous descriptors from a CPU visible descriptor heap.
		 *
		 * @param numDescriptors The number of contiguous descriptors to allocate.
		 * Cannot be more than the number of descriptors per descriptor heap.
		 */
		DescriptorAllocation Allocate(uint32 numDescriptors = 1);

		/**
		* When the frame has completed, the stale descriptors can be released.
		*/
		void ReleaseStaleDescriptors(uint64 frameNumber);

	private:
		using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

		/**
		* Creates a new heap with a specific number of descriptors.
		*/
		std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

		D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
		uint32 m_NumDescriptorsPerHeap;
		DescriptorHeapPool m_HeapPool;

		// Indices of avaliable heaps in the heap pool.
		std::set<size_t> m_AvaliableHeaps;
		
		std::mutex m_AllocationMutex;
	};
}
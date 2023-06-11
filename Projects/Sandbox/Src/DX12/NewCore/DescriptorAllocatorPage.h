#pragma once

#include "DescriptorAllocation.h"

#include "DX12/Dx12Device.h"

#include <map>
#include <memory>
#include <mutex>
#include <queue>

namespace RS
{
	class DescriptorAllocatorPage : public std::enable_shared_from_this<DescriptorAllocatorPage>
	{
	public:
		DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptors);

		D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const;

		/**
		* Check to see if this descriptor page has a contiguous block of descriptors
		* large enough to satisfy the request.
		*/
		bool HasSpace(uint32 numDescriptors) const;

		/**
		* Get the number of avaliable handles in the heap.
		*/
		uint32 NumFreeHandles() const;

		/**
		* Allocate a number of descriptors from this descriptor heap.
		* If the allocation cannot be satisfied, then a NULL descriptor is returned.
		*/
		DescriptorAllocation Allocate(uint32 numDescriptors);

		/**
		* Return a descriptor back to the heap.
		* @param frameNumber Stale descriptors are not freed directly, but put
		* on a stale allocations queue. Stale allocations are returned to the heap
		* using the DescriptorAllocatorPage::ReleaseStaleAllocations method.
		*/
		void Free(DescriptorAllocation&& descriptorHandle, uint64 frameNumber);

		/**
		* Return the stale descriptors back to the descriptor heap.
		* @param frameNumber The completed frame number.
		*/
		void ReleaseStaleDescriptors(uint64 frameNumber);

	protected:
		// Compute the offset of the descriptor handle from the start of the heap.
		uint32 ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle);

		// Adds a new block to the free list.
		void AddNewBlock(uint32 offset, uint32 numDescriptors);

		/**
		* Free a block of descriptors.
		* This will also merge free blocks in the free list to form larger blocks that can be resused.
		*/
		void FreeBlock(uint32 offset, uint32 numDescriptors);

	private:
		// The offset (in descriptors) within the descriptor heap.
		using OffsetType = uint32;

		// The number of descriptors that are available.
		using SizeType = uint32;

		struct FreeBlockInfo;
		// A map that lists the free blocks by the offset within the descriptor heap.
		using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;

		// A map that lists the free blocks by size.
		// Needs to be a multimap since multiple blocks can have the same size.
		using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;

		struct FreeBlockInfo
		{
			FreeBlockInfo(SizeType size) : size(size) {}

			SizeType size;
			FreeListBySize::iterator freeListBySizeIt; // Used for fast removals.
		};

		struct StaleDescriptorInfo
		{
			StaleDescriptorInfo(OffsetType offset, SizeType size, uint64 frame)
				: offset(offset), size(size), frameNumber(frame) {}

			// The offset within the descriptor heap.
			OffsetType offset;

			// The number of descriptors.
			SizeType size;

			// The frame number that the descriptor was freed.
			uint64 frameNumber;
		};

		// Stale descriptors are queued for release until the frame that they were freed has completed.
		using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;

		FreeListByOffset		m_FreeListByOffset;
		FreeListBySize			m_FreeListBySize;
		StaleDescriptorQueue	m_StaleDescriptors;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_d3d12DescriptorHeap;
		D3D12_DESCRIPTOR_HEAP_TYPE		m_HeapType;
		CD3DX12_CPU_DESCRIPTOR_HANDLE	m_BaseDescriptor;
		uint32							m_DescriptorHandleIncrementSize;
		uint32							m_NumDescriptorsInHeap;
		uint32							m_NumFreeHandles;

		std::mutex m_AllocationMutex;
	};
}
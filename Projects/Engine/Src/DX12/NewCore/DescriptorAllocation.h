#pragma once

#include "DX12/Dx12Device.h"

#include <memory> // std::shared_ptr

namespace RS
{
	class DescriptorAllocatorPage;
	class DescriptorAllocation
	{
	public:
		DescriptorAllocation();
		DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32 numHandles, uint32 descriptorSize, std::shared_ptr<DescriptorAllocatorPage> page);

		// The destroctor will automatically free the allocation.
		~DescriptorAllocation();

		// Copies are not allowed.
		[[noexcept]] DescriptorAllocation(const DescriptorAllocation&) = delete;
		[[noexcept]] DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;

		// Move is allowed.
		DescriptorAllocation(DescriptorAllocation&& allocation);
		DescriptorAllocation& operator=(DescriptorAllocation&& other);

		// Check if this is a valid descriptor.
		bool IsNull() const;

		// Get a descriptor at a particular offset in the allocation.
		D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32 offset = 0) const;

		// Get the number of (consecutive) handles for this allocation.
		uint32 GetNumHandles() const;

		// Get the heap that his allocation came from (for internal use only).
		std::shared_ptr<DescriptorAllocatorPage> GetDescriptorAllocatorPage() const;

	private:
		// Free the descriptor back to the heap it came from.
		void Free();

		// The base descriptor.
		D3D12_CPU_DESCRIPTOR_HANDLE m_Descriptor;

		// The number of descriptors in this allocation.
		uint32 m_NumHandles;

		// The offset to the next descriptor.
		uint32 m_DescriptorSize;

		// A pointer back to the original page where this allocation came from.
		std::shared_ptr<DescriptorAllocatorPage> m_Page;
	};
}
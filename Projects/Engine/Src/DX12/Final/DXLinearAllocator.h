//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
// 
// Modified by: Jonathan Åleskog
//
// Description:  This is a dynamic graphics memory allocator for DX12.  It's designed to work in concert
// with the CommandContext class and to do so in a thread-safe manner.  There may be many command contexts,
// each with its own linear allocators.  They act as windows into a global memory pool by reserving a
// context-local memory page.  Requesting a new page is done in a thread-safe manner by guarding accesses
// with a mutex lock.
//
// When a command context is finished, it will receive a fence ID that indicates when it's safe to reclaim
// used resources.  The CleanupUsedPages() method must be invoked at this time so that the used pages can be
// scheduled for reuse after the fence has cleared.

#pragma once

#include "DX12/Final/DXGPUResource.h"
#include <vector>
#include <queue>
#include <mutex>

// Constant blocks must be multiples of 16 constants @ 16 bytes each
#define DEFAULT_ALIGN 256

namespace RS::DX12
{
    // Various types of allocations may contain NULL pointers.  Check before dereferencing if you are unsure.
    struct DXDynAlloc
    {
        DXDynAlloc(DXGPUResource& BaseResource, size_t ThisOffset, size_t ThisSize)
            : Buffer(BaseResource), Offset(ThisOffset), Size(ThisSize) {}

        DXGPUResource& Buffer;	// The D3D buffer associated with this memory.
        size_t Offset;			// Offset from start of buffer resource
        size_t Size;			// Reserved size of this allocation
        void* DataPtr;			// The CPU-writeable address
        D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;	// The GPU-visible address
    };

    class DXLinearAllocationPage : public DXGPUResource
    {
    public:
        DXLinearAllocationPage(ID3D12Resource* pResource, D3D12_RESOURCE_STATES Usage) : DXGPUResource()
        {
            m_pResource.Attach(pResource);
            m_UsageState = Usage;
            m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();
            m_pResource->Map(0, nullptr, &m_CpuVirtualAddress);
        }

        ~DXLinearAllocationPage()
        {
            Unmap();
        }

        void Map(void)
        {
            if (m_CpuVirtualAddress == nullptr)
            {
                m_pResource->Map(0, nullptr, &m_CpuVirtualAddress);
            }
        }

        void Unmap(void)
        {
            if (m_CpuVirtualAddress != nullptr)
            {
                m_pResource->Unmap(0, nullptr);
                m_CpuVirtualAddress = nullptr;
            }
        }

        void* m_CpuVirtualAddress;
        D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
    };

    enum class DXLinearAllocatorType
    {
        Invalid = -1,

        GPUExclusive = 0,		// DEFAULT   GPU-writeable (via UAV)
        CPUWritable = 1,		// UPLOAD CPU-writeable (but write combined)

        Count
    };

    enum
    {
        GPUAllocatorPageSize = 0x10000,	// 64K
        CPUAllocatorPageSize = 0x200000	// 2MB
    };

    class DXLinearAllocatorPageManager
    {
    public:

        DXLinearAllocatorPageManager();
        DXLinearAllocationPage* RequestPage(void);
        DXLinearAllocationPage* CreateNewPage(size_t PageSize = 0);

        // Discarded pages will get recycled.  This is for fixed size pages.
        void DiscardPages(uint64_t FenceID, const std::vector<DXLinearAllocationPage*>& Pages);

        // Freed pages will be destroyed once their fence has passed.  This is for single-use,
        // "large" pages.
        void FreeLargePages(uint64_t FenceID, const std::vector<DXLinearAllocationPage*>& Pages);

        void Destroy(void) { m_PagePool.clear(); }

    private:

        static DXLinearAllocatorType sm_AutoType;

        DXLinearAllocatorType m_AllocationType;
        std::vector<std::unique_ptr<DXLinearAllocationPage> > m_PagePool;
        std::queue<std::pair<uint64_t, DXLinearAllocationPage*> > m_RetiredPages;
        std::queue<std::pair<uint64_t, DXLinearAllocationPage*> > m_DeletionQueue;
        std::queue<DXLinearAllocationPage*> m_AvailablePages;
        std::mutex m_Mutex;
    };

    class DXLinearAllocator
    {
    public:

        DXLinearAllocator(DXLinearAllocatorType Type) : m_AllocationType(Type), m_PageSize(0), m_CurOffset(~(size_t)0), m_CurPage(nullptr)
        {
            RS_ASSERT(Type > DXLinearAllocatorType::Invalid && Type < DXLinearAllocatorType::Count);
            m_PageSize = (Type == DXLinearAllocatorType::GPUExclusive ? GPUAllocatorPageSize : CPUAllocatorPageSize);
        }

        DXDynAlloc Allocate(size_t SizeInBytes, size_t Alignment = DEFAULT_ALIGN);

        void CleanupUsedPages(uint64_t FenceID);

        static void DestroyAll(void)
        {
            sm_PageManager[0].Destroy();
            sm_PageManager[1].Destroy();
        }

    private:

        DXDynAlloc AllocateLargePage(size_t SizeInBytes);

        static DXLinearAllocatorPageManager sm_PageManager[2];

        DXLinearAllocatorType m_AllocationType;
        size_t m_PageSize;
        size_t m_CurOffset;
        DXLinearAllocationPage* m_CurPage;
        std::vector<DXLinearAllocationPage*> m_RetiredPages;
        std::vector<DXLinearAllocationPage*> m_LargePageList;
    };
}

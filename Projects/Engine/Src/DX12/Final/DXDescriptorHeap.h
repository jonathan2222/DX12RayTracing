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

#pragma once

#include <mutex>
#include <vector>
#include <queue>
#include <string>

#include "DX12/Final/DX12Defines.h"

namespace RS::DX12
{
    // This is an unbounded resource descriptor allocator.  It is intended to provide space for CPU-visible
    // resource descriptors as resources are created.  For those that need to be made shader-visible, they
    // will need to be copied to a DescriptorHeap or a DynamicDescriptorHeap.
    class DXDescriptorAllocator
    {
    public:
        DXDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE Type) :
            m_Type(Type), m_CurrentHeap(nullptr), m_DescriptorSize(0)
        {
            m_CurrentHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32_t Count);

        static void DestroyAll(void);

    protected:

        static const uint32_t sm_NumDescriptorsPerHeap = 256;
        static std::mutex sm_AllocationMutex;
        static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> sm_DescriptorHeapPool;
        static ID3D12DescriptorHeap* RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type);

        D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
        ID3D12DescriptorHeap* m_CurrentHeap;
        D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentHandle;
        uint32_t m_DescriptorSize;
        uint32_t m_RemainingFreeHandles;
    };

    // This handle refers to a descriptor or a descriptor table (contiguous descriptors) that is shader visible.
    class DXDescriptorHandle
    {
    public:
        DXDescriptorHandle()
        {
            m_CpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
            m_GpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        }

        /*
        // Should we allow constructing handles that might not be shader visible?
        DescriptorHandle( D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle )
            : m_CpuHandle(CpuHandle)
        {
            m_GpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        }
        */

        DXDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle)
            : m_CpuHandle(CpuHandle), m_GpuHandle(GpuHandle)
        {
        }

        DXDescriptorHandle operator+ (INT OffsetScaledByDescriptorSize) const
        {
            DXDescriptorHandle ret = *this;
            ret += OffsetScaledByDescriptorSize;
            return ret;
        }

        void operator += (INT OffsetScaledByDescriptorSize)
        {
            if (m_CpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
                m_CpuHandle.ptr += OffsetScaledByDescriptorSize;
            if (m_GpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
                m_GpuHandle.ptr += OffsetScaledByDescriptorSize;
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE* operator&() const { return &m_CpuHandle; }
        operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return m_CpuHandle; }
        operator D3D12_GPU_DESCRIPTOR_HANDLE() const { return m_GpuHandle; }

        size_t GetCpuPtr() const { return m_CpuHandle.ptr; }
        uint64_t GetGpuPtr() const { return m_GpuHandle.ptr; }
        bool IsNull() const { return m_CpuHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
        bool IsShaderVisible() const { return m_GpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHandle;
    };

    class DXDescriptorHeap
    {
    public:

        DXDescriptorHeap(void) {}
        ~DXDescriptorHeap(void) { Destroy(); }

        void Create(const std::wstring& DebugHeapName, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxCount);
        void Destroy(void) { m_Heap = nullptr; }

        bool HasAvailableSpace(uint32_t Count) const { return Count <= m_NumFreeDescriptors; }
        DXDescriptorHandle Alloc(uint32_t Count = 1);

        DXDescriptorHandle operator[] (uint32_t arrayIdx) const { return m_FirstHandle + arrayIdx * m_DescriptorSize; }

        uint32_t GetOffsetOfHandle(const DXDescriptorHandle& DHandle) {
            return (uint32_t)(DHandle.GetCpuPtr() - m_FirstHandle.GetCpuPtr()) / m_DescriptorSize;
        }

        bool ValidateHandle(const DXDescriptorHandle& DHandle) const;

        ID3D12DescriptorHeap* GetHeapPointer() const { return m_Heap.Get(); }

        uint32_t GetDescriptorSize(void) const { return m_DescriptorSize; }

    private:

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
        D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
        uint32_t m_DescriptorSize;
        uint32_t m_NumFreeDescriptors;
        DXDescriptorHandle m_FirstHandle;
        DXDescriptorHandle m_NextFreeHandle;
    };
}

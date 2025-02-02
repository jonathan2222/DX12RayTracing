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

#include "DX12/Final/DXDescriptorHeap.h"
#include "DX12/Final/DXRootSignature.h"

#include <vector>
#include <queue>

namespace RS::DX12
{
    // This class is a linear allocation system for dynamically generated descriptor tables.  It internally caches
    // CPU descriptor handles so that when not enough space is available in the current heap, necessary descriptors
    // can be re-copied to the new heap.

    class DXCommandContext;

    class DXDynamicDescriptorHeap
    {
    public:
        DXDynamicDescriptorHeap(DXCommandContext* pOwningContext, D3D12_DESCRIPTOR_HEAP_TYPE HeapType);
        ~DXDynamicDescriptorHeap();

        static void DestroyAll(void)
        {
            sm_DescriptorHeapPool[0].clear();
            sm_DescriptorHeapPool[1].clear();
        }

        void CleanupUsedHeaps(uint64_t fenceValue);

        // Copy multiple handles into the cache area reserved for the specified root parameter.
        void SetGraphicsDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
        {
            m_GraphicsHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
        }

        void SetComputeDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
        {
            m_ComputeHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
        }

        // Bypass the cache and upload directly to the shader-visible heap
        D3D12_GPU_DESCRIPTOR_HANDLE UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE Handles);

        // Deduce cache layout needed to support the descriptor tables needed by the root signature.
        void ParseGraphicsRootSignature(const DXRootSignature& RootSig)
        {
            m_GraphicsHandleCache.ParseRootSignature(m_DescriptorType, RootSig);
        }

        void ParseComputeRootSignature(const DXRootSignature& RootSig)
        {
            m_ComputeHandleCache.ParseRootSignature(m_DescriptorType, RootSig);
        }

        // Upload any new descriptors in the cache to the shader-visible heap.
        inline void CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* CmdList)
        {
            if (m_GraphicsHandleCache.m_StaleRootParamsBitMap != 0)
                CopyAndBindStagedTables(m_GraphicsHandleCache, CmdList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
        }

        inline void CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* CmdList)
        {
            if (m_ComputeHandleCache.m_StaleRootParamsBitMap != 0)
                CopyAndBindStagedTables(m_ComputeHandleCache, CmdList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
        }

    private:

        // Static members
        static const uint32_t kNumDescriptorsPerHeap = 1024;
        static std::mutex sm_Mutex;
        static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> sm_DescriptorHeapPool[2];
        static std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>> sm_RetiredDescriptorHeaps[2];
        static std::queue<ID3D12DescriptorHeap*> sm_AvailableDescriptorHeaps[2];

        // Static methods
        static ID3D12DescriptorHeap* RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType);
        static void DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint64_t FenceValueForReset, const std::vector<ID3D12DescriptorHeap*>& UsedHeaps);

        // Non-static members
        DXCommandContext* m_pOwningContext;
        ID3D12DescriptorHeap* m_CurrentHeapPtr;
        const D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorType;
        uint32_t m_DescriptorSize;
        uint32_t m_CurrentOffset;
        DXDescriptorHandle m_FirstDescriptor;
        std::vector<ID3D12DescriptorHeap*> m_RetiredHeaps;

        // Describes a descriptor table entry:  a region of the handle cache and which handles have been set
        struct DescriptorTableCache
        {
            DescriptorTableCache() : AssignedHandlesBitMap(0) {}
            uint32_t AssignedHandlesBitMap;
            D3D12_CPU_DESCRIPTOR_HANDLE* TableStart;
            uint32_t TableSize;
        };

        struct DescriptorHandleCache
        {
            DescriptorHandleCache()
            {
                ClearCache();
            }

            void ClearCache()
            {
                m_RootDescriptorTablesBitMap = 0;
                m_StaleRootParamsBitMap = 0;
                m_MaxCachedDescriptors = 0;
            }

            uint32_t m_RootDescriptorTablesBitMap;
            uint32_t m_StaleRootParamsBitMap;
            uint32_t m_MaxCachedDescriptors;

            static const uint32_t kMaxNumDescriptors = 256;
            static const uint32_t kMaxNumDescriptorTables = 16;

            uint32_t ComputeStagedSize();
            void CopyAndBindStaleTables(D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t DescriptorSize, DXDescriptorHandle DestHandleStart, ID3D12GraphicsCommandList* CmdList,
                void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

            DescriptorTableCache m_RootDescriptorTable[kMaxNumDescriptorTables];
            D3D12_CPU_DESCRIPTOR_HANDLE m_HandleCache[kMaxNumDescriptors];

            void UnbindAllValid();
            void StageDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
            void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE Type, const DXRootSignature& RootSig);
        };

        DescriptorHandleCache m_GraphicsHandleCache;
        DescriptorHandleCache m_ComputeHandleCache;

        bool HasSpace(uint32_t Count)
        {
            return (m_CurrentHeapPtr != nullptr && m_CurrentOffset + Count <= kNumDescriptorsPerHeap);
        }

        void RetireCurrentHeap(void);
        void RetireUsedHeaps(uint64_t fenceValue);
        ID3D12DescriptorHeap* GetHeapPointer();

        DXDescriptorHandle Allocate(UINT Count)
        {
            DXDescriptorHandle ret = m_FirstDescriptor + m_CurrentOffset * m_DescriptorSize;
            m_CurrentOffset += Count;
            return ret;
        }

        void CopyAndBindStagedTables(DescriptorHandleCache& HandleCache, ID3D12GraphicsCommandList* CmdList,
            void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

        // Mark all descriptors in the cache as stale and in need of re-uploading.
        void UnbindAllValid(void);
    };
}

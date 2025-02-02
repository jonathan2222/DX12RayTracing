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

#include <vector>
#include <queue>
#include <mutex>
#include <stdint.h>
#include "DX12/Final/DXCommandAllocatorPool.h"

namespace RS::DX12
{
    class DXCommandQueue
    {
        friend class DXCommandListManager;
        friend class DXCommandContext;

    public:
        DXCommandQueue(D3D12_COMMAND_LIST_TYPE Type);
        ~DXCommandQueue();

        void Create(ID3D12Device* pDevice);
        void Shutdown();

        inline bool IsReady()
        {
            return m_CommandQueue != nullptr;
        }

        uint64_t IncrementFence(void);
        bool IsFenceComplete(uint64_t FenceValue);
        void StallForFence(uint64_t FenceValue);
        void StallForProducer(DXCommandQueue& Producer);
        void WaitForFence(uint64_t FenceValue);
        void WaitForIdle(void) { WaitForFence(IncrementFence()); }

        ID3D12CommandQueue* GetCommandQueue() { return m_CommandQueue; }

        uint64_t GetNextFenceValue() { return m_NextFenceValue; }

    private:

        uint64_t ExecuteCommandList(ID3D12CommandList* List);
        ID3D12CommandAllocator* RequestAllocator(void);
        void DiscardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator* Allocator);

        ID3D12CommandQueue* m_CommandQueue;

        const D3D12_COMMAND_LIST_TYPE m_Type;

        DXCommandAllocatorPool m_AllocatorPool;
        std::mutex m_FenceMutex;
        std::mutex m_EventMutex;

        // Lifetime of these objects is managed by the descriptor cache
        ID3D12Fence* m_pFence;
        uint64_t m_NextFenceValue;
        uint64_t m_LastCompletedFenceValue;
        HANDLE m_FenceEventHandle;

    };

    class DXCommandListManager
    {
        friend class DXCommandContext;

    public:
        DXCommandListManager();
        ~DXCommandListManager();

        void Create(ID3D12Device* pDevice);
        void Shutdown();

        DXCommandQueue& GetGraphicsQueue(void) { return m_GraphicsQueue; }
        DXCommandQueue& GetComputeQueue(void) { return m_ComputeQueue; }
        DXCommandQueue& GetCopyQueue(void) { return m_CopyQueue; }

        DXCommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT)
        {
            switch (Type)
            {
            case D3D12_COMMAND_LIST_TYPE_COMPUTE: return m_ComputeQueue;
            case D3D12_COMMAND_LIST_TYPE_COPY: return m_CopyQueue;
            default: return m_GraphicsQueue;
            }
        }

        ID3D12CommandQueue* GetCommandQueue()
        {
            return m_GraphicsQueue.GetCommandQueue();
        }

        void CreateNewCommandList(
            D3D12_COMMAND_LIST_TYPE Type,
            ID3D12GraphicsCommandList** List,
            ID3D12CommandAllocator** Allocator);

        // Test to see if a fence has already been reached
        bool IsFenceComplete(uint64_t FenceValue)
        {
            return GetQueue(D3D12_COMMAND_LIST_TYPE(FenceValue >> 56)).IsFenceComplete(FenceValue);
        }

        // The CPU will wait for a fence to reach a specified value
        void WaitForFence(uint64_t FenceValue);

        // The CPU will wait for all command queues to empty (so that the GPU is idle)
        void IdleGPU(void)
        {
            m_GraphicsQueue.WaitForIdle();
            m_ComputeQueue.WaitForIdle();
            m_CopyQueue.WaitForIdle();
        }

    private:

        ID3D12Device* m_pDevice;

        DXCommandQueue m_GraphicsQueue;
        DXCommandQueue m_ComputeQueue;
        DXCommandQueue m_CopyQueue;
    };
}

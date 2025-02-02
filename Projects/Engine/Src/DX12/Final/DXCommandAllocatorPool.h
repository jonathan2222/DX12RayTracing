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
// Modified by: Jonathan Åleskg

#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <stdint.h>

#include "DX12/Final/DX12Defines.h"

namespace RS::DX12
{
    class DXCommandAllocatorPool
    {
    public:
        DXCommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type);
        ~DXCommandAllocatorPool();

        void Create(ID3D12Device* pDevice);
        void Shutdown();

        ID3D12CommandAllocator* RequestAllocator(uint64_t CompletedFenceValue);
        void DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator);

        inline size_t Size() { return m_AllocatorPool.size(); }

    private:
        const D3D12_COMMAND_LIST_TYPE m_cCommandListType;

        ID3D12Device* m_pDevice;
        std::vector<ID3D12CommandAllocator*> m_AllocatorPool;
        std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_ReadyAllocators;
        std::mutex m_AllocatorMutex;
    };
}
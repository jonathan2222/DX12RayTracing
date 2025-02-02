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

#include "PreCompiled.h"
#include "DXCommandAllocatorPool.h"

#include "DX12/Final/DX12Defines.h"

RS::DX12::DXCommandAllocatorPool::DXCommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type) :
    m_cCommandListType(Type),
    m_pDevice(nullptr)
{
}

RS::DX12::DXCommandAllocatorPool::~DXCommandAllocatorPool()
{
    Shutdown();
}

void RS::DX12::DXCommandAllocatorPool::Create(ID3D12Device* pDevice)
{
    m_pDevice = pDevice;
}

void RS::DX12::DXCommandAllocatorPool::Shutdown()
{
    for (size_t i = 0; i < m_AllocatorPool.size(); ++i)
        m_AllocatorPool[i]->Release();

    m_AllocatorPool.clear();
}

ID3D12CommandAllocator* RS::DX12::DXCommandAllocatorPool::RequestAllocator(uint64_t CompletedFenceValue)
{
    std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);

    ID3D12CommandAllocator* pAllocator = nullptr;

    if (!m_ReadyAllocators.empty())
    {
        std::pair<uint64_t, ID3D12CommandAllocator*>& AllocatorPair = m_ReadyAllocators.front();

        if (AllocatorPair.first <= CompletedFenceValue)
        {
            pAllocator = AllocatorPair.second;
            DXCall(pAllocator->Reset());
            m_ReadyAllocators.pop();
        }
    }

    // If no allocator's were ready to be reused, create a new one
    if (pAllocator == nullptr)
    {
        DXCall(m_pDevice->CreateCommandAllocator(m_cCommandListType, IID_PPV_ARGS(&pAllocator)));
        wchar_t AllocatorName[32];
        swprintf(AllocatorName, 32, L"CommandAllocator %zu", m_AllocatorPool.size());
        pAllocator->SetName(AllocatorName);
        m_AllocatorPool.push_back(pAllocator);
    }

    return pAllocator;
}

void RS::DX12::DXCommandAllocatorPool::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator)
{
    std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);

    // That fence value indicates we are free to reset the allocator
    m_ReadyAllocators.push(std::make_pair(FenceValue, Allocator));
}

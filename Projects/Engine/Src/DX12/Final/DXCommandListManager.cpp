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
#include "DXCommandListManager.h"

#include "DX12/Final/DXCore.h"

RS::DX12::DXCommandQueue::DXCommandQueue(D3D12_COMMAND_LIST_TYPE type) :
    m_Type(type),
    m_CommandQueue(nullptr),
    m_pFence(nullptr),
    m_NextFenceValue((uint64_t)type << 56 | 1),
    m_LastCompletedFenceValue((uint64_t)type << 56),
    m_AllocatorPool(type)
{
}

RS::DX12::DXCommandQueue::~DXCommandQueue()
{
    Shutdown();
}

void RS::DX12::DXCommandQueue::Create(ID3D12Device* pDevice)
{
    RS_ASSERT(pDevice != nullptr);
    RS_ASSERT(!IsReady());
    RS_ASSERT(m_AllocatorPool.Size() == 0);

    D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
    QueueDesc.Type = m_Type;
    QueueDesc.NodeMask = 1;
    pDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&m_CommandQueue));
    m_CommandQueue->SetName(L"CommandListManager::m_CommandQueue");

    DXCall(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
    m_pFence->SetName(L"CommandListManager::m_pFence");
    m_pFence->Signal((uint64_t)m_Type << 56);

    m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
    RS_ASSERT(m_FenceEventHandle != NULL);

    m_AllocatorPool.Create(pDevice);

    RS_ASSERT(IsReady());
}

void RS::DX12::DXCommandQueue::Shutdown()
{
    if (m_CommandQueue == nullptr)
        return;

    m_AllocatorPool.Shutdown();

    CloseHandle(m_FenceEventHandle);

    m_pFence->Release();
    m_pFence = nullptr;

    m_CommandQueue->Release();
    m_CommandQueue = nullptr;
}

uint64_t RS::DX12::DXCommandQueue::IncrementFence()
{
    std::lock_guard<std::mutex> lockGuard(m_FenceMutex);
    m_CommandQueue->Signal(m_pFence, m_NextFenceValue);
    return m_NextFenceValue++;
}

bool RS::DX12::DXCommandQueue::IsFenceComplete(uint64_t fenceValue)
{
    // Avoid querying the fence value by testing against the last one seen.
    // The max() is to protect against an unlikely race condition that could cause the last
    // completed fence value to regress.
    if (fenceValue > m_LastCompletedFenceValue)
        m_LastCompletedFenceValue = std::max(m_LastCompletedFenceValue, m_pFence->GetCompletedValue());

    return fenceValue <= m_LastCompletedFenceValue;
}

void RS::DX12::DXCommandQueue::StallForFence(uint64_t FenceValue)
{
    DXCommandQueue& producer = DXCore::GetCommandListManager()->GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
    m_CommandQueue->Wait(producer.m_pFence, FenceValue);
}

void RS::DX12::DXCommandQueue::StallForProducer(DXCommandQueue& producer)
{
    RS_ASSERT(producer.m_NextFenceValue > 0);
    m_CommandQueue->Wait(producer.m_pFence, producer.m_NextFenceValue - 1);
}

void RS::DX12::DXCommandQueue::WaitForFence(uint64_t fenceValue)
{
    if (IsFenceComplete(fenceValue))
        return;

    // TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
    // wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
    // the fence can only have one event set on completion, then thread B has to wait for 
    // 100 before it knows 99 is ready.  Maybe insert sequential events?
    {
        std::lock_guard<std::mutex> lockGuard(m_EventMutex);

        m_pFence->SetEventOnCompletion(fenceValue, m_FenceEventHandle);
        WaitForSingleObject(m_FenceEventHandle, INFINITE);
        m_LastCompletedFenceValue = fenceValue;
    }
}

uint64_t RS::DX12::DXCommandQueue::ExecuteCommandList(ID3D12CommandList* pList)
{
    std::lock_guard<std::mutex> lockGuard(m_FenceMutex);

    DXCall(((ID3D12GraphicsCommandList*)pList)->Close());

    // Kickoff the command list
    m_CommandQueue->ExecuteCommandLists(1, &pList);

    // Signal the next fence value (with the GPU)
    m_CommandQueue->Signal(m_pFence, m_NextFenceValue);

    // And increment the fence value.  
    return m_NextFenceValue++;
}

ID3D12CommandAllocator* RS::DX12::DXCommandQueue::RequestAllocator()
{
    uint64_t completedFence = m_pFence->GetCompletedValue();

    return m_AllocatorPool.RequestAllocator(completedFence);
}

void RS::DX12::DXCommandQueue::DiscardAllocator(uint64_t fenceValueForReset, ID3D12CommandAllocator* pAllocator)
{
    m_AllocatorPool.DiscardAllocator(fenceValueForReset, pAllocator);
}

RS::DX12::DXCommandListManager::DXCommandListManager() :
    m_pDevice(nullptr),
    m_GraphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
    m_ComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
    m_CopyQueue(D3D12_COMMAND_LIST_TYPE_COPY)
{
}

RS::DX12::DXCommandListManager::~DXCommandListManager()
{
    Shutdown();
}

void RS::DX12::DXCommandListManager::Create(ID3D12Device* pDevice)
{
    RS_ASSERT(pDevice != nullptr);

    m_pDevice = pDevice;

    m_GraphicsQueue.Create(pDevice);
    m_ComputeQueue.Create(pDevice);
    m_CopyQueue.Create(pDevice);
}

void RS::DX12::DXCommandListManager::Shutdown()
{
    m_GraphicsQueue.Shutdown();
    m_ComputeQueue.Shutdown();
    m_CopyQueue.Shutdown();
}

void RS::DX12::DXCommandListManager::CreateNewCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList** ppList, ID3D12CommandAllocator** ppAllocator)
{
    RS_ASSERT(type != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Bundles are not yet supported");
    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT: *ppAllocator = m_GraphicsQueue.RequestAllocator(); break;
    case D3D12_COMMAND_LIST_TYPE_BUNDLE: break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE: *ppAllocator = m_ComputeQueue.RequestAllocator(); break;
    case D3D12_COMMAND_LIST_TYPE_COPY: *ppAllocator = m_CopyQueue.RequestAllocator(); break;
    }

    DXCall(DXCore::GetDevice()->CreateCommandList(1, type, *ppAllocator, nullptr, IID_PPV_ARGS(ppList)));
    (*ppList)->SetName(L"CommandList");
}

void RS::DX12::DXCommandListManager::WaitForFence(uint64_t fenceValue)
{
    DXCommandQueue& producer = DXCore::GetCommandListManager()->GetQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56));
    producer.WaitForFence(fenceValue);
}

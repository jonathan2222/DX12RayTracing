#include "PreCompiled.h"
#include "CommandQueue.h"

#include "DX12/NewCore/DX12Core3.h"

#include "Utils/Misc/ThreadUtils.h"

RS::CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type)
    : m_CommandListType(type)
    , m_FenceValue(0)
    , m_bProcessInFlightCommandLists(true)
{
    auto pDevice = DX12Core3::Get()->GetD3D12Device();

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
    DXCall(pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_d3d12CommandQueue)), "Failed to create the command queue!");
    DXCall(pDevice->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12Fence)));

    // Set the thread name for easy debugging.
    char threadName[256];
    sprintf_s(threadName, "ProcessInFlightCommandLists ");
    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        DX12_SET_DEBUG_NAME(m_d3d12CommandQueue, "Direct Command Queue");
        strcat_s(threadName, "(Direct)");
        break;
    case D3D12_COMMAND_LIST_TYPE_BUNDLE:
        DX12_SET_DEBUG_NAME(m_d3d12CommandQueue, "Bundle Command Queue");
        strcat_s(threadName, "(Bundle)");
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        DX12_SET_DEBUG_NAME(m_d3d12CommandQueue, "Compute Command Queue");
        strcat_s(threadName, "(Compute)");
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        DX12_SET_DEBUG_NAME(m_d3d12CommandQueue, "Copy Command Queue");
        strcat_s(threadName, "(Copy)");
        break;
    case D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE:
        DX12_SET_DEBUG_NAME(m_d3d12CommandQueue, "Video Decode Command Queue");
        strcat_s(threadName, "(Video Decode)");
        break;
    case D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS:
        DX12_SET_DEBUG_NAME(m_d3d12CommandQueue, "Video Process Command Queue");
        strcat_s(threadName, "(Video Process)");
        break;
    case D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE:
        DX12_SET_DEBUG_NAME(m_d3d12CommandQueue, "Video Encode Command Queue");
        strcat_s(threadName, "(Video Encode)");
        break;
    default:
        break;
    }

    m_ProcessInFlightCommandListsThread = std::thread(&CommandQueue::ProcessInFlightCommandLists, this);
    Utils::SetThreadName(m_ProcessInFlightCommandListsThread, threadName);
}

RS::CommandQueue::~CommandQueue()
{
    m_bProcessInFlightCommandLists = false;
    m_ProcessInFlightCommandListsThread.join();
}

uint64 RS::CommandQueue::Signal()
{
    uint64 fenceValueForSignal = ++m_FenceValue;
    DXCallVerbose(m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), fenceValueForSignal));
    return fenceValueForSignal;
}

bool RS::CommandQueue::IsFenceComplete(uint64 fenceValue)
{
    return m_d3d12Fence->GetCompletedValue() >= fenceValue;
}

void RS::CommandQueue::WaitForFenceValue(uint64 fenceValue)
{
    if (!IsFenceComplete(fenceValue))
    {
        HANDLE eventHandle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        RS_ASSERT(eventHandle, "Failed to create fence event handle!");

        DXCallVerbose(m_d3d12Fence->SetEventOnCompletion(fenceValue, eventHandle));
        ::WaitForSingleObject(eventHandle, DWORD_MAX);

        ::CloseHandle(eventHandle);
    }
}

void RS::CommandQueue::Flush()
{
    std::unique_lock<std::mutex> lock(m_ProcessInFlightCommandListsThreadMutex);
    m_ProcessInFlightCommandListThreadCV.wait(lock, [this] { return m_InFlightCommandLists.Empty(); });

    // In case the command queue was signaled directly using the CommandQueue::Signal method then the 
    //  fence value of the command queue might be higher than the fence value of any of the executed command lists.
    WaitForFenceValue(m_FenceValue);
}

std::shared_ptr<RS::CommandList> RS::CommandQueue::GetCommandList()
{
    std::shared_ptr<CommandList> commandList;

    // If there is a command list on the queue.
    if (!m_AvailableCommandLists.Empty())
    {
        m_AvailableCommandLists.TryPop(commandList);
    }
    else
    {
        commandList = std::make_shared<CommandList>(m_CommandListType);
    }

    return commandList;
}


uint64 RS::CommandQueue::ExecuteCommandList(std::shared_ptr<RS::CommandList> commandList)
{
    return ExecuteCommandLists({ commandList });
}

uint64 RS::CommandQueue::ExecuteCommandLists(const std::vector<std::shared_ptr<RS::CommandList>>& commandLists)
{
    ResourceStateTracker::Lock();

    // Command lists that need to be put back on the command list queue.
    std::vector<std::shared_ptr<CommandList>> toBeQueued;
    toBeQueued.reserve(commandLists.size() * 2);        // 2x since each command list will have a pending command list.

    // Command lists that need to be executed.
    std::vector<ID3D12CommandList*> d3d12CommandLists;
    d3d12CommandLists.reserve(commandLists.size() * 2); // 2x since each command list will have a pending command list.

    for (auto commandList : commandLists)
    {
        auto pendingCommandList = GetCommandList();
        bool hasPendingBarriers = commandList->Close(*pendingCommandList);
        pendingCommandList->Close();

        // If there are no pending barriers on the pending command list, there is no reason to execute an empty command list on the command queue.
        if (hasPendingBarriers)
            d3d12CommandLists.push_back(pendingCommandList->GetGraphicsCommandList().Get());
        d3d12CommandLists.push_back(commandList->GetGraphicsCommandList().Get());

        toBeQueued.push_back(pendingCommandList);
        toBeQueued.push_back(commandList);
    }

    UINT numCommandLists = static_cast<UINT>(d3d12CommandLists.size());
    m_d3d12CommandQueue->ExecuteCommandLists(numCommandLists, d3d12CommandLists.data());
    uint64 fenceValue = Signal();

    ResourceStateTracker::Unlock();

    for (auto commandList : toBeQueued)
        m_InFlightCommandLists.Push({ fenceValue, commandList });

    return fenceValue;
}

void RS::CommandQueue::Wait(const CommandQueue& other)
{
    m_d3d12CommandQueue->Wait(other.m_d3d12Fence.Get(), other.m_FenceValue);
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> RS::CommandQueue::GetD3D12CommandQueue() const
{
    return m_d3d12CommandQueue;
}

void RS::CommandQueue::ProcessInFlightCommandLists()
{
    std::unique_lock<std::mutex> lock(m_ProcessInFlightCommandListsThreadMutex, std::defer_lock);

    while (m_bProcessInFlightCommandLists)
    {
        CommandListEntry commandListEntry;

        lock.lock();
        while (m_InFlightCommandLists.TryPop(commandListEntry))
        {
            auto fenceValue = std::get<0>(commandListEntry);
            auto commandList = std::get<1>(commandListEntry);

            WaitForFenceValue(fenceValue);

            commandList->Reset();

            m_AvailableCommandLists.Push(commandList);
        }
        lock.unlock();
        m_ProcessInFlightCommandListThreadCV.notify_one();

        std::this_thread::yield();
    }
}

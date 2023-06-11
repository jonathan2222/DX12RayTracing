#include "PreCompiled.h"
#include "CommandQueue.h"

RS::CommandQueue::CommandQueue(Microsoft::WRL::ComPtr<DX12_DEVICE_TYPE> device, D3D12_COMMAND_LIST_TYPE type)
    : m_d3d12Device(device)
    , m_CommandListType(type)
    , m_FenceValue(0)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
    DXCallVerbose(m_d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_d3d12CommandQueue)), "Failed to create the command queue!");
    DXCall(m_d3d12Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12Fence)));

    m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

    RS_ASSERT(m_FenceEvent, "Failed to create fence event handle!");
}

RS::CommandQueue::~CommandQueue()
{
    Flush();
}

std::shared_ptr<RS::CommandList> RS::CommandQueue::GetCommandList()
{
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    std::shared_ptr<CommandList> commandList;

    // A command allocator cannot be used until the command has been fully executed on the GPU.
    if (!m_CommandAllocatorQueue.empty() && IsFenceComplete(m_CommandAllocatorQueue.front().fenceValue))
    {
        commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
        m_CommandAllocatorQueue.pop();
        DXCall(commandAllocator->Reset());
    }
    else
    {
        commandAllocator = CreateCommandAllocator();
    }

    // A command queue can be used directly after it has been submitted.
    if (!m_CommandListQueue.empty())
    {
        commandList = m_CommandListQueue.front();
        m_CommandListQueue.pop();
        commandList->Reset();
    }
    else
    {
        commandList = CreateCommandList(commandAllocator);
    }

    // Associate the command allocator with the command list so that it can be retrieved when the command list is executed.
    DXCall(commandList->GetGraphicsCommandList()->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

    return commandList;
}

uint64 RS::CommandQueue::ExecuteCommandList(std::shared_ptr<RS::CommandList> commandList)
{
    // Close the command list such that it can be executed.
    commandList->Close();

    ID3D12CommandAllocator* commandAllocator = nullptr;

    UINT dataSize = sizeof(commandAllocator);
    DXCall(commandList->GetGraphicsCommandList()->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, commandAllocator));

    ID3D12CommandList* const ppComandLists[] = { commandList->GetGraphicsCommandList().Get() };
    m_d3d12CommandQueue->ExecuteCommandLists(1, ppComandLists);

    uint64 fenceValue = Signal();

    m_CommandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
    m_CommandListQueue.push(commandList);

    // The ownership of the command allocator has been transferred to the ComPtr in the command allocator queue.
    // It is safe to release the reference in this temporary COM pointer here.
    commandAllocator->Release();

    return fenceValue;
}

uint64 RS::CommandQueue::Signal()
{
    uint64 fenceValueForSignal = ++m_FenceValue;
    DXCall(m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), fenceValueForSignal));
    return fenceValueForSignal;
}

bool RS::CommandQueue::IsFenceComplete(uint64 fenceValue)
{
    return m_d3d12Fence->GetCompletedValue() >= fenceValue;
}

void RS::CommandQueue::WaitForFenceValue(uint64 fenceValue)
{
    static std::chrono::milliseconds duration = std::chrono::milliseconds::max();
    if (m_d3d12Fence->GetCompletedValue() < fenceValue)
    {
        DXCall(m_d3d12Fence->SetEventOnCompletion(fenceValue, m_FenceEvent));
        ::WaitForSingleObject(m_FenceEvent, static_cast<DWORD>(duration.count()));
    }
}

void RS::CommandQueue::Flush()
{
    uint64 fenceValueForSignal = Signal();
    WaitForFenceValue(fenceValueForSignal);
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> RS::CommandQueue::GetD3D12CommandQueue() const
{
    return m_d3d12CommandQueue;
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> RS::CommandQueue::CreateCommandAllocator()
{
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    DXCallVerbose(m_d3d12Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&commandAllocator)), "Failed to create the command allocator!");
    return commandAllocator;
}

std::shared_ptr<RS::CommandList> RS::CommandQueue::CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator)
{
    return std::make_unique<CommandList>(m_CommandListType, allocator);
}

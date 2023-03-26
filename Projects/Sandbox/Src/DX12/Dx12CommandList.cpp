#include "PreCompiled.h"
#include "Dx12CommandList.h"

void RS::DX12::Dx12FrameCommandList::Init(DX12_DEVICE_PTR pDevice, D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0; // For multiple devices.
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Type = type;
	DXCall(pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue)));
	DX12_SET_DEBUG_NAME(m_CommandQueue, "{} Command Queue", DX12::GetCommandListTypeAsString(type).data());

	for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		CommandFrame& commandFrame = m_CommandFrames[i];
		commandFrame.Init(pDevice, type);
		DX12_SET_DEBUG_NAME(commandFrame.m_CommandAllocator, "{} Command Allocator [{}]", DX12::GetCommandListTypeAsString(type), i);
		LOG_DEBUG("Created command allocator [{}] of type {}", i, DX12::GetCommandListTypeAsString(type).data());
	}

	m_QueueFenceValue = 0;
	DXCall(pDevice->CreateCommandList(m_QueueFenceValue, type, m_CommandFrames[0].m_CommandAllocator, nullptr, IID_PPV_ARGS(&m_CommandList)));
	DX12_SET_DEBUG_NAME(m_CommandList, "{} Command List", DX12::GetCommandListTypeAsString(type).data());
	DXCall(m_CommandList->Close());

	DXCall(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_QueueFence)));
	DX12_SET_DEBUG_NAME(m_QueueFence, "D3D12 Fence");

	m_CPUFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
	RS_ASSERT(m_CPUFenceEvent, "Failed to create windows event!");

}

void RS::DX12::Dx12FrameCommandList::Release()
{
	// Wait for gpu tasks to finish.
	Flush();

	// We need to wait for the present operation too. This is because EndFrame signals before present was called.
	WaitForGPUQueue();

	DX12_RELEASE(m_QueueFence);
	m_QueueFenceValue = 0;
	
	if (m_CPUFenceEvent)
	{
		CloseHandle(m_CPUFenceEvent);
		m_CPUFenceEvent = nullptr;
	}

	DX12_RELEASE(m_CommandQueue);
	DX12_RELEASE(m_CommandList);

	for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
		m_CommandFrames[i].Release();
}

void RS::DX12::Dx12FrameCommandList::BeginFrame()
{
	CommandFrame& commandFrame = m_CommandFrames[m_FrameIndex];
	commandFrame.Wait(m_QueueFence, m_CPUFenceEvent);

	DXCallVerbose(commandFrame.m_CommandAllocator->Reset());
	DXCallVerbose(m_CommandList->Reset(commandFrame.m_CommandAllocator, nullptr));
}

void RS::DX12::Dx12FrameCommandList::EndFrame()
{
	DXCallVerbose(m_CommandList->Close());

	ID3D12CommandList* commandLists[]{ m_CommandList };
	m_CommandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

	// Tell GPU to signal the fence when done executing the command lists.
	CommandFrame& commandFrame = m_CommandFrames[m_FrameIndex];
	commandFrame.m_FenceValue = ++m_QueueFenceValue;
	DXCallVerbose(m_CommandQueue->Signal(m_QueueFence, commandFrame.m_FenceValue));
}

void RS::DX12::Dx12FrameCommandList::MoveToNextFrame()
{
	m_FrameIndex = (m_FrameIndex + 1) % FRAME_BUFFER_COUNT;
}

void RS::DX12::Dx12FrameCommandList::Flush()
{
	for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		m_CommandFrames[i].Wait(m_QueueFence, m_CPUFenceEvent);
	}
	m_FrameIndex = 0;
}

void RS::DX12::Dx12FrameCommandList::WaitForGPUQueue()
{
	uint64 fenceValue = ++m_QueueFenceValue;
	DXCall(m_CommandQueue->Signal(m_QueueFence, fenceValue));

	// We have the fence create an event which is signaled when the fence's current value equals m_FenceValue.
	DXCallVerbose(m_QueueFence->SetEventOnCompletion(fenceValue, m_CPUFenceEvent));

	// Wait until the fence has triggered the event.
	WaitForSingleObjectEx(m_CPUFenceEvent, INFINITE, FALSE);
}

void RS::DX12::Dx12FrameCommandList::CommandFrame::Init(ID3D12Device8* pDevice, D3D12_COMMAND_LIST_TYPE type)
{
	DXCall(pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&m_CommandAllocator)));
}

void RS::DX12::Dx12FrameCommandList::CommandFrame::Release()
{
	DX12_RELEASE(m_CommandAllocator);
	m_FenceValue = 0;
}

void RS::DX12::Dx12FrameCommandList::CommandFrame::Wait(ID3D12Fence1* pFence, HANDLE fenceEvent)
{
	RS_ASSERT(pFence && fenceEvent, "Fence or fence handle are not initialized!");

	// If the current fence value on the GPU is less than the one on the CPU, then we know the GPU has not finished executing the command lists sice it has not reached the "m_CommandQueue->Signal()" command.
	if (pFence->GetCompletedValue() < m_FenceValue)
	{
		// We have the fence create an event which is signaled when the fence's current value equals m_FenceValue.
		DXCallVerbose(pFence->SetEventOnCompletion(m_FenceValue, fenceEvent));

		// Wait until the fence has triggered the event.
		WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
	}
}

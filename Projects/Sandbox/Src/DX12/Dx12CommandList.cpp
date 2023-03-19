#include "PreCompiled.h"
#include "Dx12CommandList.h"

void RS::DX12::Dx12FrameCommandList::Init(ID3D12Device8* pDevice, D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0; // For multiple devices.
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Type = type;
	HRESULT hr = pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue));
	ThrowIfFailed(hr, "Failed to create {} command queue!", DX12::GetCommandListTypeAsString(type).data());
	DX12_SET_DEBUG_NAME(m_CommandQueue, "{} Command Queue", DX12::GetCommandListTypeAsString(type).data());

	for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		CommandFrame& commandFrame = m_CommandFrames[i];
		commandFrame.Init(pDevice, type);
	}

	hr = pDevice->CreateCommandList(0, type, m_CommandFrames[0].m_CommandAllocator, nullptr, IID_PPV_ARGS(&m_CommandList));
	ThrowIfFailed(hr, "Failed to create {} command list!", DX12::GetCommandListTypeAsString(type).data());
	DX12_SET_DEBUG_NAME(m_CommandList, "{} Command List", DX12::GetCommandListTypeAsString(type).data());

	hr = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
	ThrowIfFailed(hr, "Failed to create fence!");
	DX12_SET_DEBUG_NAME(m_Fence, "D3D12 Fence");

	m_FenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
	RS_ASSERT(m_FenceEvent, "Failed to create windows event!");

}

void RS::DX12::Dx12FrameCommandList::Release()
{
	// Wait for gpu tasks to finish.
	Flush();

	DX12_RELEASE(m_Fence);
	m_FenceValue = 0;
	
	if (m_FenceEvent)
	{
		CloseHandle(m_FenceEvent);
		m_FenceEvent = nullptr;
	}

	DX12_RELEASE(m_CommandQueue);
	DX12_RELEASE(m_CommandList);

	for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
		m_CommandFrames[i].Release();
}

void RS::DX12::Dx12FrameCommandList::BeginFrame()
{
	CommandFrame& commandFrame = m_CommandFrames[m_FrameIndex];
	commandFrame.Wait(m_Fence, m_FenceEvent);

	HRESULT hr = commandFrame.m_CommandAllocator->Reset();
	ThrowIfFailed(hr, "Failed to reset command allocator!");

	hr = m_CommandList->Reset(commandFrame.m_CommandAllocator, nullptr);
	ThrowIfFailed(hr, "Failed to reset command list!");
}

void RS::DX12::Dx12FrameCommandList::EndFrame()
{
	HRESULT hr = m_CommandList->Close();
	ThrowIfFailed(hr, "Failed to close command list!");

	ID3D12CommandList* commandLists[]{ m_CommandList };
	m_CommandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), &commandLists[0]);

	// Tell GPU to signal the fence when done executing the command lists.
	++m_FenceValue;
	CommandFrame& commandFrame = m_CommandFrames[m_FrameIndex];
	commandFrame.m_FenceValue = m_FenceValue;
	m_CommandQueue->Signal(m_Fence, m_FenceValue);

	m_FrameIndex = (m_FrameIndex + 1) % FRAME_BUFFER_COUNT;
}

void RS::DX12::Dx12FrameCommandList::Flush()
{
	for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		m_CommandFrames[i].Wait(m_Fence, m_FenceEvent);
	}
	m_FrameIndex = 0;
}

void RS::DX12::Dx12FrameCommandList::CommandFrame::Init(ID3D12Device8* pDevice, D3D12_COMMAND_LIST_TYPE type)
{
	HRESULT hr = pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&m_CommandAllocator));
	ThrowIfFailed(hr, "Failed to create {} command allocator!", DX12::GetCommandListTypeAsString(type));
	DX12_SET_DEBUG_NAME(m_CommandAllocator, "{} Command Allocator", DX12::GetCommandListTypeAsString(type));
}

void RS::DX12::Dx12FrameCommandList::CommandFrame::Release()
{
	DX12_RELEASE(m_CommandAllocator);
	m_FenceValue = 0;
}

void RS::DX12::Dx12FrameCommandList::CommandFrame::Wait(ID3D12Fence1* pFence, HANDLE fenceEvent)
{
	RS_ASSERT(pFence, fenceEvent);

	// If the current fence value on the GPU is less than the one on the CPU, then we know the GPU has not finished executing the command lists sice it has not reached the "m_CommandQueue->Signal()" command.
	if (pFence->GetCompletedValue() < m_FenceValue)
	{
		// We have the fence create an event which is signaled ones the fence's current value equals m_FenceValue.
		HRESULT hr = pFence->SetEventOnCompletion(m_FenceValue, fenceEvent);
		ThrowIfFailed(hr, "Failed to set EventOnCompletion");

		// Wait until the fence has triggered the event.
		WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
	}
}

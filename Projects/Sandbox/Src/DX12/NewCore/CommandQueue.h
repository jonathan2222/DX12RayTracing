#pragma once

#include "DX12/Dx12Device.h"

#include "DX12/NewCore/CommandList.h"

#include <queue>

namespace RS
{
	/* Usage:
	* commandQueue = std::make_shared(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	* 
	* auto commandList = commandQueue->GetCommandList();
	* // Record commands....
	* auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	* 
	* // This is not needed if it is ok for the commands to run the next frame(s) too.
	* commandQueue->WaitForFenceValue(fenceValue);
	* 
	* Usage (present):
	* commandQueue = std::make_shared(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	* auto commandList = commandQueue->GetCommandList();
	* // Transition to state render target.
	* // Record commands....
	* // Transition to state present.
	* fenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);
	* currentBackBufferIndex = Present();
	* // Waits for the next back buffer to be done.
	* commandQueue->WaitForFenceValue(fenceValues[currentBackBufferIndex]);
	*/

	class CommandQueue
	{
	public:
		CommandQueue(Microsoft::WRL::ComPtr<DX12_DEVICE_TYPE> device, D3D12_COMMAND_LIST_TYPE type);
		virtual ~CommandQueue();

		// Get an available command list from the command queue.
		std::shared_ptr<CommandList> GetCommandList();
	
		// Execute a command list.
		// Returns the fence value to wait for this command list.
		uint64 ExecuteCommandList(std::shared_ptr<RS::CommandList> commandList);

		uint64 Signal();

		bool IsFenceComplete(uint64 fenceValue);
		void WaitForFenceValue(uint64 fenceValue);
		
		void Flush();

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

	protected:
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
		std::shared_ptr<RS::CommandList> CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator);

	private:
		// Keep track of command allocators that are "in-flight".
		struct CommandAllocatorEntry
		{
			uint64 fenceValue;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
		};

		using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
		using CommandListQueue = std::queue<std::shared_ptr<CommandList>>;

		D3D12_COMMAND_LIST_TYPE						m_CommandListType;
		Microsoft::WRL::ComPtr<ID3D12Device2>		m_d3d12Device;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>	m_d3d12CommandQueue;
		Microsoft::WRL::ComPtr<ID3D12Fence>			m_d3d12Fence;
		HANDLE										m_FenceEvent;
		uint64										m_FenceValue;

		CommandAllocatorQueue						m_CommandAllocatorQueue;
		CommandListQueue							m_CommandListQueue;
	};
}
#pragma once

#include "DX12/Dx12Device.h"

#include "DX12/NewCore/CommandList.h"

#include "DX12/NewCore/ThreadSafeQueue.h"

#include <tuple>
#include <thread>
#include <condition_variable>
#include <mutex>

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
		CommandQueue(D3D12_COMMAND_LIST_TYPE type);
		virtual ~CommandQueue();

		// Get an available command list from the command queue.
		std::shared_ptr<CommandList> GetCommandList();
	
		// Execute a command list.
		// Returns the fence value to wait for this command list.
		uint64 ExecuteCommandList(std::shared_ptr<RS::CommandList> commandList);
		uint64 ExecuteCommandLists(const std::vector<std::shared_ptr<RS::CommandList>>& commandLists);

		uint64 Signal();
		bool IsFenceComplete(uint64 fenceValue);
		void WaitForFenceValue(uint64 fenceValue);
		void Flush();

		// Wait for another command queue to finish.
		void Wait(const CommandQueue& other);

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

	private:
		// Free any command lists that are finished processing on the command queue.
		void ProcessInFlightCommandLists();

		using CommandListEntry = std::tuple<uint64, std::shared_ptr<CommandList>>;

		D3D12_COMMAND_LIST_TYPE							m_CommandListType;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>		m_d3d12CommandQueue;
		Microsoft::WRL::ComPtr<ID3D12Fence>				m_d3d12Fence;
		uint64											m_FenceValue;

		ThreadSafeQueue<CommandListEntry>				m_InFlightCommandLists;
		ThreadSafeQueue<std::shared_ptr<CommandList>>	m_AvailableCommandLists;

		// A thread to process in-flight command lists.
		std::thread m_ProcessInFlightCommandListsThread;
		std::atomic_bool m_bProcessInFlightCommandLists;
		std::mutex m_ProcessInFlightCommandListsThreadMutex;
		std::condition_variable	m_ProcessInFlightCommandListThreadCV;
	};
}
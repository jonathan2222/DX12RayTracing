#pragma once

#include "Dx12Device.h"

namespace RS::DX12
{
	class Dx12FrameCommandList
	{
	public:
		RS_NO_COPY_AND_MOVE(Dx12FrameCommandList);
		Dx12FrameCommandList() = default;
		~Dx12FrameCommandList() = default;

		void Init(DX12_DEVICE_PTR pDevice, D3D12_COMMAND_LIST_TYPE type);
		void Release();

		void BeginFrame();
		void EndFrame();
		void MoveToNextFrame();

		void Flush();
		void WaitForGPUQueue();

		constexpr ID3D12CommandQueue* const GetCommandQueue() const { return m_CommandQueue; }
		constexpr ID3D12GraphicsCommandList* const GetCommandList() const { return m_CommandList; }
		constexpr uint32 GetFrameIndex() const { return m_FrameIndex; }

	private:
		struct CommandFrame
		{
			ID3D12CommandAllocator* m_CommandAllocator = nullptr;
			uint64 m_FenceValue = 0;
			
			void Init(ID3D12Device8* pDevice, D3D12_COMMAND_LIST_TYPE type);
			void Release();
			void Wait(ID3D12Fence1* pFence, HANDLE fenceEvent);
		};

	private:
		ID3D12GraphicsCommandList*		m_CommandList = nullptr;
		CommandFrame					m_CommandFrames[FRAME_BUFFER_COUNT]{};
		ID3D12CommandQueue*				m_CommandQueue = nullptr;

		uint64							m_QueueFenceValue = 0;
		ID3D12Fence1*					m_QueueFence = nullptr;
		HANDLE							m_CPUFenceEvent = nullptr;

		uint32							m_FrameIndex = 0; // Bound by FRAME_BUFFER_COUNT
	};
}
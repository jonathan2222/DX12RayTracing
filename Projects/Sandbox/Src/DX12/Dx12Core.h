#pragma once

#include "DX12/DX12Defines.h"

namespace RS
{
	class Dx12Core
	{
	public:
		Dx12Core() = default;
		~Dx12Core() = default;

		static std::shared_ptr<Dx12Core> Get();

		void Init(uint32 width, uint32 height, HWND hwnd);
		void Release();

		void WaitForGPU();
		void MoveToNextFrame();

		// --- Temp functions ---
		void LoadAssets();
		void Render();
		void PopulateCommandList();

	private:
		/* 
			The meaning of s_FrameCount is for use in both the maximum number of frames that will be queued to the GPU at a time, 
			as well as the number of back buffers in the DXGI swap chain. For the majority of applications, this is convenient and works well.
		*/
		static const UINT s_FrameCount= 2;

		struct Vertex
		{
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT4 color;
		};

		// Pipeline objects.
		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;
		ComPtr<IDXGISwapChain3> m_swapChain;
		ComPtr<ID3D12Device> m_device;
		ComPtr<ID3D12Resource> m_renderTargets[s_FrameCount];
		ComPtr<ID3D12CommandAllocator> m_commandAllocators[s_FrameCount];
		ComPtr<ID3D12CommandQueue> m_commandQueue;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		ComPtr<ID3D12PipelineState> m_pipelineState;
		ComPtr<ID3D12GraphicsCommandList> m_commandList;
		UINT m_rtvDescriptorSize;

		// App resources.
		ComPtr<ID3D12Resource> m_vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

		// Synchronization objects.
		UINT m_frameIndex;
		HANDLE m_fenceEvent;
		ComPtr<ID3D12Fence> m_fence;
		UINT64 m_fenceValues[s_FrameCount];
	};
}
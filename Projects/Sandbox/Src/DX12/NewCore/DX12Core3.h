#pragma once

#include "DX12/Dx12Device.h"
#include "DX12/Dx12Surface.h"

#include "DX12/NewCore/CommandQueue.h"
#include "DX12/NewCore/DescriptorAllocator.h"
#include "DX12/NewCore/SwapChain.h"

#include <mutex>
#include <vector>


// TODO: Move these
#include "DX12/NewCore/RootSignature.h"

namespace RS
{
	class DX12Core3
	{
	public:
		RS_NO_COPY_AND_MOVE(DX12Core3);
		virtual ~DX12Core3();

		static DX12Core3* Get();

		void Init(HWND window, int width, int height);
		void Release();

		void FreeResource(Microsoft::WRL::ComPtr<ID3D12Resource> pResource);

		bool WindowSizeChanged(uint32 width, uint32 height, bool isFullscreen, bool windowed, bool minimized);

		// TODO: This function should be moved to a main renderer or the EngineLoop.
		void Render();

		void ReleaseStaleDescriptors();

		uint32 GetCurrentFrameIndex() const;
		DX12_DEVICE_PTR GetD3D12Device() const { return m_Device.GetD3D12Device(); }
		DX12_FACTORY_PTR GetDXGIFactory() const { return m_Device.GetDXGIFactory(); }

		CommandQueue* GetDirectCommandQueue() const { return m_pDirectCommandQueue.get(); }

		void Flush();

		DescriptorAllocator* GetDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
		DescriptorAllocation AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type);

	private:
		DX12Core3();

		void ReleasePendingResourceRemovals(uint32 frameIndex);

		// TODO: Move these!
		void CreatePipelineState();
		void CreateRootSignature();

	private:
		std::mutex m_ResourceRemovalMutex;
		std::unordered_map<uint64, std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>> m_PendingResourceRemovals;

		uint32 m_CurrentFrameIndex = 0;

		// TODO: Remake these.
		DX12::Dx12Device				m_Device;
		// Would like to have multiple of these such that we can have more windows open.
		//DX12::Dx12Surface				m_Surface; // TODO: This should not be called surface. Maybe canvas or swap chain?
		std::unique_ptr<SwapChain>		m_pSwapChain;
		std::unique_ptr<CommandQueue>	m_pDirectCommandQueue;

		std::unique_ptr<DescriptorAllocator> m_pDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

		uint64 m_FenceValues[FRAME_BUFFER_COUNT];

		// TODO: Move these!
		std::shared_ptr<RootSignature> m_pRootSignature;
		ID3D12PipelineState* m_pPipelineState = nullptr;
		std::shared_ptr<VertexBuffer> m_pVertexBufferResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_ConstantBufferResource;
		std::shared_ptr<Texture> m_NullTexture;
		std::shared_ptr<Texture> m_NormalTexture;

		struct RootParameter
		{
			static const uint32 PixelData = 0;
			static const uint32 PixelData2 = 1;

			// TODO: Same Table
			static const uint32 Textures = 2;
			static const uint32 ConstantBufferViews = 3;
			static const uint32 UnordedAccessViews = 4;
		};
	};
}
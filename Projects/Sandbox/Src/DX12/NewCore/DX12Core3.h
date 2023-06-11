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

		// TODO: This function should be moved to a main renderer or the EngineLoop.
		void Render();

		void ReleaseStaleDescriptors();

		uint32 GetCurrentFrameIndex() const;
		DX12_DEVICE_PTR GetD3D12Device() const { return m_Device.GetD3D12Device(); }
		DX12_FACTORY_PTR GetDXGIFactory() const { return m_Device.GetDXGIFactory(); }

		DescriptorAllocator* GetShaderResourceDescriptorAllocator() { return m_pShaderResourceDescriptorHeap.get(); };
		DescriptorAllocator* GetSamplerDescriptorAllocator() { return m_pSamplerDescriptorHeap.get(); };
		DescriptorAllocator* GetRTVDescriptorAllocator() { return m_pRTVDescriptorHeap.get(); };
		DescriptorAllocator* GetDSVDescriptorAllocator() { return m_pDSVDescriptorHeap.get(); };

		CommandQueue* GetDirectCommandQueue() const { return m_pDirectCommandQueue.get(); }

		void Flush();

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
		DX12::Dx12Surface				m_Surface; // TODO: This should not be called surface. Maybe canvas or swap chain?
		std::unique_ptr<SwapChain>		m_SwapChain;
		std::unique_ptr<CommandQueue>	m_pDirectCommandQueue;

		std::unique_ptr<DescriptorAllocator> m_pShaderResourceDescriptorHeap;
		std::unique_ptr<DescriptorAllocator> m_pSamplerDescriptorHeap;
		std::unique_ptr<DescriptorAllocator> m_pRTVDescriptorHeap;
		std::unique_ptr<DescriptorAllocator> m_pDSVDescriptorHeap;

		uint64 m_FenceValues[FRAME_BUFFER_COUNT];

		// TODO: Move these!
		std::unique_ptr<RootSignature> m_pRootSignature;
		ID3D12PipelineState* m_pPipelineState = nullptr;
	};
}
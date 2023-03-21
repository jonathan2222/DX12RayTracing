#pragma once

#include <mutex>

#include "Dx12Device.h"
#include "Dx12CommandList.h"
#include "Dx12Resources.h"

namespace RS::DX12
{
	class Dx12Core2
	{
	public:
		RS_NO_COPY_AND_MOVE(Dx12Core2);
		Dx12Core2() = default;
		~Dx12Core2() = default;

		static Dx12Core2* Get();

		void Init(HWND window, int width, int height);
		void Release();

		void Render();

		ID3D12Device8* GetD3D12Device() { return m_Device.GetD3D12Device(); }
		IDXGIFactory4* GetDXGIFactory() { return m_Device.GetDXGIFactory(); }
		
		uint32 GetCurrentFrameIndex() const { return m_FrameCommandList.GetFrameIndex(); };
		void SetDeferredReleasesFlag(uint32 frameIndex) { m_DeferredReleasesFlags[frameIndex] = 1; }

		const Dx12FrameCommandList* GetFrameCommandList() const { return &m_FrameCommandList; }

		template<typename T>
		void DeferredRelease(T*& resouce);
	private:
		void ProcessDeferredReleases(uint32 frameIndex);

	private:
		Dx12Device				m_Device;
		Dx12FrameCommandList	m_FrameCommandList;
		HWND					m_Window = nullptr;

		// Deferred releases of resources.
		std::mutex				m_DeferredReleasesMutex;
		uint32					m_DeferredReleasesFlags[FRAME_BUFFER_COUNT]{};
		std::vector<IUnknown*>	m_DeferredReleases[FRAME_BUFFER_COUNT]{};

		// Resources
		Dx12DescriptorHeap		m_DescriptorHeapRTV{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV };
		Dx12DescriptorHeap		m_DescriptorHeapDSV{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
		Dx12DescriptorHeap		m_DescriptorHeapSRV{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
		Dx12DescriptorHeap		m_DescriptorHeapUAV{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
	};

	template<typename T>
	inline void Dx12Core2::DeferredRelease(T*& resource)
	{
		if (resource)
		{
			const uint32 frameIndex = GetCurrentFrameIndex();
			std::lock_guard lock{ m_DeferredReleasesMutex };
			m_DeferredReleases[frameIndex].push_back(dynamic_cast<IUnknown*>(resource));
			SetDeferredReleasesFlag(frameIndex);
			resource = nullptr;
		}
	}
}
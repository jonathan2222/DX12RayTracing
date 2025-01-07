#pragma once

#include "DX12/Dx12Device.h"

#include "DX12/NewCore/CommandQueue.h"
#include "DX12/NewCore/DescriptorAllocator.h"
#include "DX12/NewCore/SwapChain.h"
#include "DX12/NewCore/Resources.h"

#include <mutex>
#include <vector>

// TODO: Move these
#include "Utils/Timer.h"

#include "DX12/NewCore/RootSignature.h"

namespace RS
{
	// Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
	interface IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

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

		SwapChain* GetSwapChain() const { return m_pSwapChain.get(); }

		CommandQueue* GetDirectCommandQueue() const { return m_pDirectCommandQueue.get(); }

		/*
		* Flushes the current 
		*/
		void Flush();
		void WaitForGPU();

		DescriptorAllocator* GetDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
		DescriptorAllocation AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type);

		// Debug code
		static void AddToLifetimeTracker(uint64 id, const std::string& name = "");
		static void RemoveFromLifetimeTracker(uint64 id);
		static void SetNameOfLifetimeTrackedResource(uint64 id, const std::string& name);
		static void LogLifetimeTracker();

		std::array<uint32, FRAME_BUFFER_COUNT> GetNumberOfPendingPremovals();

	private:
		DX12Core3();

		void ReleasePendingResourceRemovals(uint32 frameIndex);

	private:
		std::mutex m_ResourceRemovalMutex;
		std::unordered_map<uint64, std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>> m_PendingResourceRemovals;

		uint32 m_CurrentFrameIndex = 0;

		// TODO: Remake this.
		DX12::Dx12Device				m_Device;
		// Would like to have multiple of these such that we can have more windows open.
		std::unique_ptr<SwapChain>		m_pSwapChain; // These resources will not be released before we call Release()!
		std::unique_ptr<CommandQueue>	m_pDirectCommandQueue; // These resources will not be released before we call Release()!

		std::unique_ptr<DescriptorAllocator> m_pDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

		uint64 m_FenceValues[FRAME_BUFFER_COUNT];

	public:
		// Resource lifetime tracking debug code (-logResources)
		inline static std::mutex s_ResourceLifetimeTrackingMutex;
		struct LifetimeStats
		{
			char name[256];
			uint64 startTime;
			LifetimeStats() : name(""), startTime(0) {}
			LifetimeStats(const std::string& name)
			{
				startTime = Timer::GetCurrentTimeSeconds();
				if (name.empty())
					strcpy_s(this->name, "Unknown Resource");
				else
					strcpy_s(this->name, name.c_str());
			}
		};
		inline static std::unordered_map<uint64, LifetimeStats> s_ResourceLifetimeTrackingData;

		void ResizeTexture(const std::shared_ptr<Texture>& pTexture, uint32 newWidth, uint32 newHeight);
	private:
		// Resize stuff
		std::shared_ptr<RootSignature> m_ResizeTextureRootSignature;
		ID3D12PipelineState* m_ResizeTexturePipelineState = nullptr;
	};
}
#pragma once

#include "DX12/DX12Defines.h"

namespace RS
{
	// Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
	interface IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	class Dx12Core
	{
	public:
		// Options
		static const unsigned int c_AllowTearing = 0x1;
		static const unsigned int c_RequireTearingSupport = 0x2;

		Dx12Core() = default;
		~Dx12Core() = default;

		static std::shared_ptr<Dx12Core> Get();

		void Init(
			DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
			DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
			uint32 backBufferCount = 2,
			D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0,
			uint32 flags = 0,
			UINT adapterIDoverride= UINT_MAX);

		//void Init(uint32 width, uint32 height, HWND hwnd);
		void Release();

		void InitializeDXGIAdapter();
		void SetAdapterOverride(UINT adapterID) { m_AdapterIDoverride = adapterID; }
		void CreateDeviceResources();
		void CreateWindowSizeDependentResources();
		void SetWindow(HWND window, int width, int height);
		bool WindowSizeChanged(int width, int height, bool minimized);
		void HandleDeviceLost();
		void RegisterDeviceNotify(IDeviceNotify* pDeviceNotify)
		{
			m_DeviceNotify = pDeviceNotify;

			// On RS4 and higher, applications that handle device removal 
			// should declare themselves as being able to do so
			__if_exists(DXGIDeclareAdapterRemovalSupport)
			{
				if (pDeviceNotify)
				{
					if (FAILED(DXGIDeclareAdapterRemovalSupport()))
					{
						LOG_ERROR("Application failed to declare adapter removal support!");
					}
				}
			}
		}

		void Prepare(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT);
		void Present(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET);
		void ExecuteCommandList();
		void WaitForGpu() noexcept;

		//void WaitForGPU();
		//void MoveToNextFrame();

		// --- Temp functions ---
		//void LoadAssets();
		//void Render();
		//void PopulateCommandList();

		// Device Accessors.
		RECT GetOutputSize() const { return m_OutputSize; }
		bool IsWindowVisible() const { return m_IsWindowVisible; }
		bool IsTearingSupported() const { return m_Options & c_AllowTearing; }

		// Direct3D Accessors.
		IDXGIAdapter1*				GetAdapter() const { return m_Adapter.Get(); }
		ID3D12Device*				GetD3DDevice() const { return m_D3DDevice.Get(); }
		IDXGIFactory4*				GetDXGIFactory() const { return m_DXGIFactory.Get(); }
		IDXGISwapChain3*			GetSwapChain() const { return m_SwapChain.Get(); }
		D3D_FEATURE_LEVEL           GetDeviceFeatureLevel() const { return m_D3DFeatureLevel; }
		ID3D12Resource*				GetRenderTarget() const { return m_RenderTargets[m_BackBufferIndex].Get(); }
		ID3D12Resource*				GetDepthStencil() const { return m_DepthStencil.Get(); }
		ID3D12CommandQueue*			GetCommandQueue() const { return m_CommandQueue.Get(); }
		ID3D12CommandAllocator*		GetCommandAllocator() const { return m_CommandAllocators[m_BackBufferIndex].Get(); }
		ID3D12GraphicsCommandList*	GetCommandList() const { return m_CommandList.Get(); }
		DXGI_FORMAT                 GetBackBufferFormat() const { return m_BackBufferFormat; }
		DXGI_FORMAT                 GetDepthBufferFormat() const { return m_DepthBufferFormat; }
		D3D12_VIEWPORT              GetScreenViewport() const { return m_ScreenViewport; }
		D3D12_RECT                  GetScissorRect() const { return m_ScissorRect; }
		UINT                        GetCurrentFrameIndex() const { return m_BackBufferIndex; }
		UINT                        GetPreviousFrameIndex() const { return m_BackBufferIndex == 0 ? m_BackBufferCount - 1 : m_BackBufferIndex - 1; }
		UINT                        GetBackBufferCount() const { return m_BackBufferCount; }
		unsigned int                GetDeviceOptions() const { return m_Options; }
		LPCWSTR                     GetAdapterDescription() const { return m_AdapterDescription.c_str(); }
		UINT                        GetAdapterID() const { return m_AdapterID; }

		CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_BackBufferIndex, m_RTVDescriptorSize);
		}
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		}

		// Returns bool whether the device supports DirectX Raytracing tier.
		static bool IsDirectXRaytracingSupported(IDXGIAdapter1* adapter)
		{
			ComPtr<ID3D12Device> testDevice;
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};

			return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))
				&& SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
				&& featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
		}

	private:
		//ComPtr<IDXGIFactory4> CreateFactory();
		//void CreateDevice(ComPtr<IDXGIFactory4>& factory, D3D_FEATURE_LEVEL featureLevel);

		void MoveToNextFrame();
		void InitializeAdapter(IDXGIAdapter1** ppAdapter);

	private:
		/* 
			The meaning of s_FrameCount is for use in both the maximum number of frames that will be queued to the GPU at a time, 
			as well as the number of back buffers in the DXGI swap chain. For the majority of applications, this is convenient and works well.
		*/
		const static size_t MAX_BACK_BUFFER_COUNT = 3;

		//struct Vertex
		//{
		//	DirectX::XMFLOAT3 position;
		//	DirectX::XMFLOAT4 color;
		//};

		UINT                                                m_AdapterIDoverride;
		UINT                                                m_BackBufferIndex;
		ComPtr<IDXGIAdapter1>                               m_Adapter;
		UINT                                                m_AdapterID;
		std::wstring                                        m_AdapterDescription;

		// Direct3D objects.
		Microsoft::WRL::ComPtr<ID3D12Device>				m_D3DDevice;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_CommandQueue;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_CommandList;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_CommandAllocators[MAX_BACK_BUFFER_COUNT];

		// Swap chain objects.
		Microsoft::WRL::ComPtr<IDXGIFactory4>               m_DXGIFactory;
		Microsoft::WRL::ComPtr<IDXGISwapChain3>             m_SwapChain;
		Microsoft::WRL::ComPtr<ID3D12Resource>              m_RenderTargets[MAX_BACK_BUFFER_COUNT];
		Microsoft::WRL::ComPtr<ID3D12Resource>              m_DepthStencil;

		// Direct3D rendering objects.
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_RTVDescriptorHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_DSVDescriptorHeap;
		UINT                                                m_RTVDescriptorSize;
		D3D12_VIEWPORT                                      m_ScreenViewport;
		D3D12_RECT                                          m_ScissorRect;

		// Synchronization objects.
		Microsoft::WRL::Wrappers::Event						m_FenceEvent;
		ComPtr<ID3D12Fence>									m_Fence;
		UINT64												m_FenceValues[MAX_BACK_BUFFER_COUNT];

		// Direct3D properties.
		DXGI_FORMAT											m_BackBufferFormat;
		DXGI_FORMAT											m_DepthBufferFormat;
		UINT												m_BackBufferCount;
		D3D_FEATURE_LEVEL									m_D3DMinFeatureLevel;

		// Cached device properties.
		HWND                                                m_Window;
		D3D_FEATURE_LEVEL                                   m_D3DFeatureLevel;
		RECT                                                m_OutputSize;
		bool                                                m_IsWindowVisible;

		// DeviceResources options (see flags above)
		unsigned int                                        m_Options;

		// The IDeviceNotify can be held directly as it owns the DeviceResources.
		IDeviceNotify*										m_DeviceNotify;
	};
}
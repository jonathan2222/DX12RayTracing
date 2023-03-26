#pragma once

#include "Dx12Device.h"
#include "Dx12Resources.h"

namespace RS::DX12
{
	class Dx12Surface
	{
	public:
		void Init(HWND window, uint32 width, uint32 height, DXGIFlags flags);
		void ReleaseResources();
		void Release();

		void PrepareDraw(ID3D12GraphicsCommandList* pCommandList);
		void EndDraw(ID3D12GraphicsCommandList* pCommandList);
		void Present();

		void Resize(uint32 width, uint32 height, bool isFullscreen);

		struct RenderTarget
		{
			Dx12DescriptorHandle	handle;
			ID3D12Resource*			pResource = nullptr;
			D3D12_RESOURCE_STATES	state = D3D12_RESOURCE_STATE_COMMON;

#ifdef RS_CONFIG_DEBUG
			std::string				debugName = "Unnamed SwapChain RT";
#endif
		};
		const RenderTarget& GetRenderTarget(uint32 index) { return m_RenderTargets[index]; }
		const RenderTarget& GetCurrentRenderTarget() { return m_RenderTargets[m_BackBufferIndex]; }
		uint32 GetCurrentBackBufferIndex() const { return m_BackBufferIndex; }

		uint32 GetWidth() const { return m_Width; }
		uint32 GetHeight() const { return m_Height; }

	private:
		void CreateSwapChain(HWND window, uint32 width, uint32 height, DXGI_FORMAT format, DXGIFlags flags);
		void CreateResources(DXGI_FORMAT format, Dx12DescriptorHeap* pRTVDescHeap);

	private:
		IDXGISwapChain3*	m_SwapChain = nullptr;
		RenderTarget		m_RenderTargets[FRAME_BUFFER_COUNT]{};

		bool				m_UseVSync = false;
		bool				m_IsFullscreen = false;
		uint32				m_BackBufferIndex = 0;
		uint32				m_Width = 0;
		uint32				m_Height = 0;
		DXGIFlags			m_Flags = DXGIFlag::NONE;
		DXGI_FORMAT			m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	};
}
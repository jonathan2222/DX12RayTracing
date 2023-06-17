#pragma once

#include "DX12/Dx12Device.h"
#include "DX12/NewCore/RenderTarget.h"

namespace RS
{
	class SwapChain
	{
	public:
		inline static const uint32 BackBufferCount = 3;

	public:
		explicit SwapChain(HWND window, uint32 width, uint32 height, DX12::DXGIFlags flags);
		virtual ~SwapChain();

		// Return the next back buffer index.
		uint32 Present(const std::shared_ptr<Texture>& texture);

		void Resize(uint32 width, uint32 height, bool isFullscreen);

		std::shared_ptr<Texture> GetBackBuffer(uint32 index) { return m_BackBufferTextures[index]; }
		std::shared_ptr<Texture> GetCurrentBackBuffer() { return m_BackBufferTextures[m_BackBufferIndex]; }
		uint32 GetCurrentBackBufferIndex() const { return m_BackBufferIndex; }

		uint32 GetWidth() const { return m_Width; }
		uint32 GetHeight() const { return m_Height; }
		DXGI_FORMAT GetFormat() const { return m_Format; }

	private:
		void CreateSwapChain(HWND window, uint32 width, uint32 height, DXGI_FORMAT format, DX12::DXGIFlags flags);
		void CreateResources(DXGI_FORMAT format);

	private:
		Microsoft::WRL::ComPtr<IDXGISwapChain3>	m_SwapChain;
		std::shared_ptr<Texture>				m_BackBufferTextures[BackBufferCount]{ nullptr };

		bool				m_UseVSync			= false;
		bool				m_IsFullscreen		= false;
		uint32				m_BackBufferIndex	= BackBufferCount - 1;
		uint32				m_Width				= 0;
		uint32				m_Height			= 0;
		DX12::DXGIFlags		m_Flags				= DX12::DXGIFlag::NONE;
		DXGI_FORMAT			m_Format			= DXGI_FORMAT_R8G8B8A8_UNORM;

		uint64				m_FenceValues[BackBufferCount];
	};
}
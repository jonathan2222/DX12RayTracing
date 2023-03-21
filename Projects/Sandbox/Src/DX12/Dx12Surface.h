#pragma once

#include "Dx12Device.h"

namespace RS::DX12
{
	class Dx12Surface
	{
	public:
		void Init(HWND window, uint32 width, uint32 height, DXGIFlags flags);
		void Release();

	private:
		void CreateSwapChain(HWND window, uint32 width, uint32 height, DXGI_FORMAT format, DXGIFlags flags);

	private:
		IDXGISwapChain3*	m_SwapChain = nullptr;
		ID3D12Resource*		m_RenderTargets[FRAME_BUFFER_COUNT]{ nullptr };

		uint32				m_Width = 0;
		uint32				m_Height = 0;
		DXGIFlags			m_Flags = DXGIFlag::NONE;
		DXGI_FORMAT			m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	};
}
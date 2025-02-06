#pragma once

#include "DX12/Final/DX12Defines.h"
#include "DX12/Final/DXColorBuffer.h"
#include "DX12/Final/DXRootSignature.h"
#include "DX12/Final/DXPipelineState.h"
#include "Core/Display.h"

#include "Utils/Timer.h"

#define SWAP_CHAIN_BUFFER_COUNT 3

namespace RS::DX12
{
	class DXDisplay : public IDisplaySizeChange
	{
	public:
		DXDisplay();

		void Init(HWND hWnd, uint32 width, uint32 height, DXGI_FORMAT format = DXGI_FORMAT_R10G10B10A2_UNORM);
		void Remove();

		void Present(DXColorBuffer* pBase);

		void Resize(uint32 width, uint32 height);

		void OnSizeChange(uint32 width, uint32 height, bool isFullscreen, bool windowed) override;

	private:
		void PresentSDR(DXColorBuffer* pBase);

	private:
		DXColorBuffer m_DisplayPlanes[SWAP_CHAIN_BUFFER_COUNT];
		IDXGISwapChain1* m_pSwapChain1 = nullptr;
		DXGI_FORMAT m_SwapChainFormat;
		uint m_CurrentBuffer = 0;
		uint m_FrameIndex = 0;

		DXRootSignature m_PresentRootSignature;
		DXGraphicsPSO m_PresentSDRPSO;
		DXGraphicsPSO m_ScalePresentSDRPSO;
		DXGraphicsPSO m_BlendUIPSO;

		float m_FrameTime;
		uint64 m_FrameStartTick = 0;

		uint32 m_Width;
		uint32 m_Height;

		// Settings
		bool m_EnableVSync = true;
		bool m_EnableHDROutput = false;
		bool m_LimitTo30Hz = false;
	};
}
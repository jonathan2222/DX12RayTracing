#pragma once

#include "DX12/Final/DX12Defines.h"
#include "DX12/Final/DXColorBuffer.h"
#include "DX12/Final/DXRootSignature.h"
#include "DX12/Final/DXPipelineState.h"
#include "Core/Display.h"

#include "Utils/Timer.h"

namespace RS::DX12
{
	class DXGraphicsContext;
	class DXDisplay : public IDisplaySizeChange
	{
	public:
		inline static constexpr uint BufferCount = 3;

	public:
		DXDisplay();

		void Init(HWND hWnd, uint32 width, uint32 height, DXGI_FORMAT format = DXGI_FORMAT_R10G10B10A2_UNORM);
		void Remove();

		// Returns the new buffer index
		uint Present(DXColorBuffer* pBase, DXGraphicsContext* pContext = nullptr);

		void Resize(uint32 width, uint32 height);

		void OnSizeChange(uint32 width, uint32 height, bool isFullscreen, bool windowed) override;

		DXGI_FORMAT GetFormat() const { return m_SwapChainFormat; }

		uint GetCurrentBufferIndex() const { return m_CurrentBufferIndex; }
		uint GetCurrentFrameIndex() const { return m_FrameIndex; }

		// Thread safe
		void FreeResource(Microsoft::WRL::ComPtr<ID3D12Resource> pResource);

	private:
		void PresentSDR(DXColorBuffer* pBase, DXGraphicsContext* pContext);

		void DeletePendingRemovals(uint bufferIndex);
		void DeleteAllPendingRemovals();

	private:
		DXColorBuffer m_DisplayPlanes[BufferCount];
		IDXGISwapChain1* m_pSwapChain1 = nullptr;
		DXGI_FORMAT m_SwapChainFormat;
		uint m_CurrentBufferIndex = 0;
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

		// Remove resource each time Present is called.
		std::mutex m_PendingRemovalsMutex;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_PendingRemovalsPerDisplayBuffer[BufferCount];
	};
}
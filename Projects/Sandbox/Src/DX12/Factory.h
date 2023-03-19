#pragma once

#include "DX12/DX12Defines.h"

namespace RS::DX12
{
	class DX12Factory
	{
	public:
		/*
		* Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
		* If no such adapter can be found, *ppAdapter will be set to nullptr.
		* From: D3D12HelloWorld DirectX-Graphics-Samples
		*/
		static void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter = false);

		static void CreateSwapChain(
			ComPtr<IDXGIFactory4>& factory,
			ComPtr<ID3D12CommandQueue>& commandQueue,
			HWND hwnd,
			uint32 numBackBuffers,
			uint32 width,
			uint32 height,
			ComPtr<IDXGISwapChain1>& swapChain);

	private:

	};
}
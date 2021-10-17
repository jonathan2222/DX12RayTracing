#pragma once

#include "DX12/DX12Defines.h"

namespace RS
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

	private:

	};
}
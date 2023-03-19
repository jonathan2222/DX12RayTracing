#pragma once

#include "DX12/DX12Defines.h"

namespace RS
{
	BEGIN_BITFLAGS_U32(DXGIFlags)
		BITFLAG(ALLOW_TEARING)
		BITFLAG(REQUIRE_TEARING_SUPPORT)
	END_BITFLAGS(DXGIFlags)

	class Dx12Device
	{
	public:
		void Init(D3D_FEATURE_LEVEL minFeatureLevel, uint32 dxgiFlags);
		void Release();

	private:
		void CreateFactory();
		void FetchDX12Adapter(IDXGIAdapter1** ppAdapter);
		void CreateDevice();

	private:
		IDXGIAdapter1*	m_Adapter = nullptr;
		IDXGIFactory4*	m_Factory = nullptr;
		ID3D12Device*	m_Device = nullptr;

		uint32 m_DxgiFlags = 0;
		uint32 m_AdapterID = 0;
		D3D_FEATURE_LEVEL	m_D3DMinFeatureLevel;
		D3D_FEATURE_LEVEL	m_D3DFeatureLevel = D3D_FEATURE_LEVEL_1_0_CORE;
	};
}
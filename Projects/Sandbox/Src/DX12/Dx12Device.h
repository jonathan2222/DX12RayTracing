#pragma once

#include "DX12/DX12Defines.h"

namespace RS::DX12
{
	BEGIN_BITFLAGS_U32(DXGIFlag)
		BITFLAG(ALLOW_TEARING)
		BITFLAG(REQUIRE_TEARING_SUPPORT)
	END_BITFLAGS();

	class Dx12Device
	{
	public:
		RS_NO_COPY_AND_MOVE(Dx12Device);
		Dx12Device() = default;
		~Dx12Device() = default;

		void Init(D3D_FEATURE_LEVEL minFeatureLevel, DXGIFlags dxgiFlags);
		void Release();

		DX12_DEVICE_PTR GetD3D12Device() const { return m_Device; }
		DX12_FACTORY_PTR GetDXGIFactory() const { return m_Factory; }

		DXGIFlags GetDXGIFlags() const { return m_DxgiFlags; }

	private:
		void CreateFactory();
		void FetchDX12Adapter(IDXGIAdapter1** ppAdapter);
		void CreateDevice();

	private:
		IDXGIAdapter1*		m_Adapter = nullptr;
		DX12_FACTORY_PTR	m_Factory = nullptr;
		DX12_DEVICE_PTR		m_Device = nullptr;

		DXGIFlags			m_DxgiFlags = 0;
		uint32				m_AdapterID = 0;
		D3D_FEATURE_LEVEL	m_D3DMinFeatureLevel;
		D3D_FEATURE_LEVEL	m_D3DFeatureLevel = D3D_FEATURE_LEVEL_1_0_CORE;
	};
}
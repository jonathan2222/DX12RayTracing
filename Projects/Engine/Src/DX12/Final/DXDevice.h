#pragma once

#include "DX12/Final/DX12Defines.h"

namespace RS::DX12
{
	class DXDevice
	{
	public:
		RS_ENUM_FLAGS(uint32, DXGIFlag,
			ALLOW_TEARING,
			REQUIRE_TEARING_SUPPORT // Shouldn't this also include ALLOW_TEARING?
		);

	public:
		RS_NO_COPY_AND_MOVE(DXDevice);
		DXDevice() = default;
		~DXDevice() = default;

		void Init(D3D_FEATURE_LEVEL minFeatureLevel, DXGIFlags dxgiFlags);
		void Release();

		DX12_DEVICE_PTR GetD3D12Device() const { return m_pDevice; }
		DX12_FACTORY_PTR GetDXGIFactory() const { return m_pFactory; }

		DXGIFlags GetDXGIFlags() const { return m_DxgiFlags; }

		operator DX12_DEVICE_TYPE& () { RS_ASSERT(m_pDevice); return *m_pDevice; }

	private:
		void CreateFactory();
		void FetchDX12Adapter(IDXGIAdapter1** ppAdapter);
		void CreateDevice();

	private:
		IDXGIAdapter1*		m_pAdapter = nullptr;
		DX12_FACTORY_PTR	m_pFactory = nullptr;
		DX12_DEVICE_PTR		m_pDevice = nullptr;

		DXGIFlags			m_DxgiFlags = 0;
		uint32				m_AdapterID = 0;
		D3D_FEATURE_LEVEL	m_D3DMinFeatureLevel;
		D3D_FEATURE_LEVEL	m_D3DFeatureLevel = D3D_FEATURE_LEVEL_1_0_CORE;
	};
}
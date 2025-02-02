#include "PreCompiled.h"
#include "DXCore.h"

#include "DXCommandContext.h"
#include "DXRootSignature.h"
#include "DXDescriptorHeap.h"

void RS::DX12::DXCore::Init()
{
	m_spContextManager = new DXContextManager();

	const D3D_FEATURE_LEVEL d3dMinFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	const DX12::DXDevice::DXGIFlags dxgiFlags = DX12::DXDevice::DXGIFlag::REQUIRE_TEARING_SUPPORT;
	m_sDevice.Init(d3dMinFeatureLevel, dxgiFlags);

	m_sCommandListManager.Create(GetDevice());
}

void RS::DX12::DXCore::Release()
{
	m_sCommandListManager.IdleGPU();

	DXCommandContext::DestroyAllContexts();
	m_sCommandListManager.Shutdown();
	DXRootSignature::DestroyAll();
	DXDescriptorAllocator::DestroyAll();

	m_sDevice.Release();

	delete m_spContextManager;
	m_spContextManager = nullptr;
}

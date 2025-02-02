#pragma once

#include "DX12/Final/DXDevice.h"
#include "DX12/Final/DXCommandListManager.h"
#include "DX12/Final/DXDescriptorHeap.h" // DXDescriptorAllocator

namespace RS::DX12
{
	class DXContextManager;
	class DXCore
	{
	public:
		static void Init();
		static void Release();

		static DX12_DEVICE_TYPE* GetDevice() {
			return m_sDevice.GetD3D12Device();
		};

		static DXContextManager* GetContextManager() {
			return m_spContextManager;
		};

		static DXCommandListManager* GetCommandListManager()
		{
			return &m_sCommandListManager;
		}

		inline static D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count = 1)
		{
			return m_sDescriptorAllocator[type].Allocate(count);
		}

	private:
		inline static DXDevice m_sDevice;
		inline static DXCommandListManager m_sCommandListManager;
		inline static DXContextManager* m_spContextManager = nullptr;

		inline static DXDescriptorAllocator m_sDescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
		{
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV
		};
	};
}
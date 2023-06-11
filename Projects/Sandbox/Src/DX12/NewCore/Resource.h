#pragma once

#include "DX12/Dx12Device.h"
#include "DX12/NewCore/DescriptorAllocation.h"

namespace RS
{
	class CommandList;
	class Resource
	{
	public:
		Resource();
		virtual ~Resource();

		Microsoft::WRL::ComPtr<ID3D12Resource> GetD3D12Resource() const { return m_pD3D12Resource; }

		D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const;

		D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const;

		void Free() const;

		void SetName(const std::string& name);
		std::string GetName() const;

		void SetDescriptor(DescriptorAllocation&& descriptorAllocation);

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const { return m_DescriptorAllocation.GetDescriptorHandle(); }

		D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const;

	protected:
		friend class CommandList;

		Microsoft::WRL::ComPtr<ID3D12Resource> m_pD3D12Resource;
		mutable bool m_WasFreed;

		DescriptorAllocation m_DescriptorAllocation; // Releases the descriptor back to the pool when the destructor is called.

		std::string m_Name;
	};
}
#pragma once

#include "DX12/Dx12Device.h"
#include "DX12/NewCore/DescriptorAllocation.h"

namespace RS
{
	class CommandList;
	class Resource
	{
	public:
		Microsoft::WRL::ComPtr<ID3D12Resource> GetD3D12Resource() const { return m_pD3D12Resource; }

		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const = 0;
		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const = 0;

		void Free() const;

		void SetName(const std::string& name);
		std::string GetName() const;

		//void SetDescriptor(DescriptorAllocation&& descriptorAllocation);

		//D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const { return m_DescriptorAllocation.GetDescriptorHandle(); }

		D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const;

		bool CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const;
		bool CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const;

		bool IsValid() const;

		D3D12_CLEAR_VALUE* GetClearValue() const;

	protected:
		Resource(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* pClearValue = nullptr, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);
		Resource(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, const D3D12_CLEAR_VALUE* pClearValue = nullptr);
		virtual ~Resource();

		D3D12_RESOURCE_DESC CheckFeatureSupport();

		// Used for the inherited resource to implement the Get functions.
		DescriptorAllocation CreateShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const;
		DescriptorAllocation CreateUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const;

	private:
		uint64 GenerateID() const;

	protected:
		friend class CommandList;

		D3D12_CLEAR_VALUE* m_pD3D12ClearValue = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_pD3D12Resource;
		mutable bool m_WasFreed;

		std::string m_Name;

		D3D12_FEATURE_DATA_FORMAT_SUPPORT m_FormatSupport;

		inline static std::mutex s_IDGeneratorMutex;
		inline static uint64 s_IDGenerator = 0;
		uint64 m_ID;
	};
}
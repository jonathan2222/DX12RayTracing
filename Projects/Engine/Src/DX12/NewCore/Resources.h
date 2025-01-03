#pragma once

#include "DX12/NewCore/Resource.h"
#include <mutex>
#include <unordered_map>

namespace RS
{
	class Buffer : public Resource
	{
	public:
		Buffer(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* pClearValue, const std::string& name, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);
		Buffer(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, const std::string& name);
		virtual ~Buffer();

		virtual void CreateViews();

		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;
		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

		bool Map(uint32 subresource, const D3D12_RANGE* pReadRange, void** ppData);
		void Unmap(uint32 subresource, const D3D12_RANGE* pWrittenRange);
		bool Map(uint32 subresource, void** ppData);
		void Unmap(uint32 subresource);

	private:
		mutable std::unordered_map<size_t, DescriptorAllocation> m_ShaderResourceViews;
		mutable std::unordered_map<size_t, DescriptorAllocation> m_UnorderedAccessViews;

		mutable std::mutex m_ShaderResourceViewsMutex;
		mutable std::mutex m_UnorderedAccessViewsMutex;
	};

	class VertexBuffer : public Buffer
	{
	public:
		VertexBuffer(uint32 stride, const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* pClearValue, const std::string& name, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);
		VertexBuffer(uint32 stride, Microsoft::WRL::ComPtr<ID3D12Resource> pResource, const std::string& name);
		virtual ~VertexBuffer();

		D3D12_VERTEX_BUFFER_VIEW CreateView() const;

		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override
		{
			RS_ASSERT(false, "Vertex buffer does not support SRVs!");
			return D3D12_CPU_DESCRIPTOR_HANDLE{};
		}
		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override
		{
			RS_ASSERT(false, "Vertex buffer does not support UAVs!");
			return D3D12_CPU_DESCRIPTOR_HANDLE{};
		}

		uint64 GetSize() const
		{
			return m_SizeInBytes;
		}

		uint64 GetStride() const
		{
			return m_StrideInBytes;
		}

		uint32 GetCount() const
		{
			return m_SizeInBytes / m_StrideInBytes;
		}

	private:
		uint32 m_SizeInBytes;
		uint32 m_StrideInBytes;
	};

	class IndexBuffer : public Buffer
	{
	public:
		IndexBuffer(bool is32Bit, const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* pClearValue, const std::string& name, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);
		IndexBuffer(bool is32Bit, Microsoft::WRL::ComPtr<ID3D12Resource> pResource, const std::string& name);
		virtual ~IndexBuffer();

		D3D12_INDEX_BUFFER_VIEW CreateView() const;

		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override
		{
			RS_ASSERT(false, "Vertex buffer does not support SRVs!");
			return D3D12_CPU_DESCRIPTOR_HANDLE{};
		}
		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override
		{
			RS_ASSERT(false, "Vertex buffer does not support UAVs!");
			return D3D12_CPU_DESCRIPTOR_HANDLE{};
		}

		uint64 GetSize() const
		{
			return m_SizeInBytes;
		}

	private:
		uint32		m_SizeInBytes;
		DXGI_FORMAT m_Format;
	};

	class Texture : public Resource
	{
	public:
		Texture(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* pClearValue, const std::string& name);
		Texture(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, const std::string& name);
		virtual ~Texture() {}

		// Will remove the old resource and create a new with the updated size.
		void Resize(uint32 width, uint32 height, uint32 depthOrArraySize);
		// Will remove the old resource and create a new with the updated size.
		void Resize(uint32 width, uint32 height);

		/**
		* Create SRV and UAVs for the resource.
		*/
		virtual void CreateViews();

		/**
		* Get the SRV for a resource.
		*/
		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

		/**
		* Get the UAV for a (sub)resource.
		*/
		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

		/**
		 * Get the RTV for the texture.
		 */
		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;

		/**
		 * Get the DSV for the texture.
		 */
		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

		// Move these to another class/namespace?
		static bool CheckSRVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
		{
			return ((formatSupport & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE) != 0 ||
				(formatSupport & D3D12_FORMAT_SUPPORT1_SHADER_LOAD) != 0);
		}

		static bool CheckRTVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
		{
			return ((formatSupport & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0);
		}

		static bool CheckUAVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
		{
			return ((formatSupport & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) != 0);
		}

		static bool CheckDSVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
		{
			return ((formatSupport & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0);
		}
		static bool IsUAVCompatibleFormat(DXGI_FORMAT format);
		static bool IsSRGBFormat(DXGI_FORMAT format);
		static bool IsBGRFormat(DXGI_FORMAT format);
		static bool IsDepthFormat(DXGI_FORMAT format);
		static DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format);

	private:
		mutable std::unordered_map<size_t, DescriptorAllocation> m_ShaderResourceViews;
		mutable std::unordered_map<size_t, DescriptorAllocation> m_UnorderedAccessViews;

		mutable std::mutex m_ShaderResourceViewsMutex;
		mutable std::mutex m_UnorderedAccessViewsMutex;

		DescriptorAllocation m_RenderTargetViewAllocation;
		DescriptorAllocation m_DepthStencilAllocation;
	};
}
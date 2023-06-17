#pragma once

#include "DX12/NewCore/Resource.h"
#include <mutex>
#include <unordered_map>

namespace RS
{
	class Buffer : public Resource
	{
	public:
		Buffer(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* pClearValue, const std::string& name);
		Buffer(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, const std::string& name);
		virtual ~Buffer();

	private:
	};

	class VertexBuffer : public Buffer
	{
	public:
		VertexBuffer(uint32 stride, const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* pClearValue, const std::string& name);
		VertexBuffer(uint32 stride, Microsoft::WRL::ComPtr<ID3D12Resource> pResource, const std::string& name);
		virtual ~VertexBuffer();

		D3D12_VERTEX_BUFFER_VIEW CreateView() const;

	private:
		uint32 m_SizeInBytes;
		uint32 m_StrideInBytes;
	};

	class Texture : public Resource
	{
	public:
		Texture(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* pClearValue, const std::string& name);
		Texture(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, const std::string& name);
		virtual ~Texture() {}

		void Resize(uint32 width, uint32 height, uint32 depthOrArraySize);

		/**
		* Create SRV and UAVs for the resource.
		*/
		virtual void CreateViews();

		/**
		* Get the SRV for a resource.
		*
		* @param dxgiFormat The required format of the resource. When accessing a
		* depth-stencil buffer as a shader resource view, the format will be different.
		*/
		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const;

		/**
		* Get the UAV for a (sub)resource.
		*/
		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const;

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
		DescriptorAllocation CreateShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const;
		DescriptorAllocation CreateUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const;

		mutable std::unordered_map<size_t, DescriptorAllocation> m_ShaderResourceViews;
		mutable std::unordered_map<size_t, DescriptorAllocation> m_UnorderedAccessViews;

		mutable std::mutex m_ShaderResourceViewsMutex;
		mutable std::mutex m_UnorderedAccessViewsMutex;

		DescriptorAllocation m_RenderTargetViewAllocation;
		DescriptorAllocation m_DepthStencilAllocation;
	};
}
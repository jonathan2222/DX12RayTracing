#pragma once

#include "DX12/Dx12Device.h"

#include "DX12/NewCore/Resource.h"
#include "DX12/NewCore/ResourceStateTracker.h"
#include "DX12/NewCore/UploadBuffer.h"
#include "DX12/NewCore/DynamicDescriptorHeap.h"
#include "DX12/NewCore/RenderTarget.h"

#define DX12_COMMAND_LIST_TYPE ID3D12GraphicsCommandList2

namespace RS
{
	// This is more like a RenderContext.
	// One for each thread.
	class CommandList
	{
	public:
		explicit CommandList(D3D12_COMMAND_LIST_TYPE type, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator);
		virtual ~CommandList();

		void Reset();

		void Close();

		Resource* CreateBufferResource();

		void TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, D3D12_RESOURCE_STATES stateAfter, UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
		void TransitionBarrier(const std::shared_ptr<Resource>& pResource, D3D12_RESOURCE_STATES stateAfter, UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
		void UAVBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, bool flushBarriers);
		void UAVBarrier(const std::shared_ptr<Resource>& pResource, bool flushBarriers);
		void AliasingBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> pResourceBefore, Microsoft::WRL::ComPtr<ID3D12Resource> pResourceAfter, bool flushBarriers);
		void AliasingBarrier(const std::shared_ptr<Resource>& pResourceBefore, const std::shared_ptr<Resource>& pResourceAfter, bool flushBarriers);
		void FlushResourceBarriers();

		void TrackResource(Microsoft::WRL::ComPtr<ID3D12Object> object);
		void TrackResource(const std::shared_ptr<Resource>& resource);
		void ReleaseTrackedObjects();

		void CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> dstResource, Microsoft::WRL::ComPtr<ID3D12Resource> srcResource);
		void CopyResource(const std::shared_ptr<Resource>& dstResource, const std::shared_ptr<Resource>& srcResource);

		Microsoft::WRL::ComPtr<ID3D12Resource> CopyBuffer(size_t bufferSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags);

		void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* pHeap) {}

		void SetGraphicsDynamicConstantBuffer(uint32 rootParameterIndex, size_t sizeInBytes, const void* bufferData);

		/** Binds a resource as an SRV.
		* @param rootParameterIndex The root parameter index to assign the SRV to. The root parameter must be of type D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE.
		* @param descriptorOffset The offset starting from the first descriptor in the D3D12_ROOT_DESCRIPTOR_TABLE. This must refer to a descriptor in a D3D12_DESCRIPTOR_RANGE of type D3D12_DESCRIPTOR_RANGE_TYPE_SRV.
		* @param resource The resource to bind.
		* @param stateAfter The required state of the resource.
		* @param firstSubresource The first subresource to transition to the requested state.
		* @param numSubresources The number of subresources to transition.
		* @param srv The SRV description to use for the resource in the shader.
		*/
		void SetShaderResourceView(
			uint32 rootParameterIndex,
			uint32 descriptorOffset,
			const std::shared_ptr<Resource>& pResource,
			D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			UINT firstSubresource = 0,
			UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			const D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr);

		/** Binds a resource as an UAV.
		* @param rootParameterIndex The root parameter index to assign the UAV to. The root parameter must be of type D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE.
		* @param descriptorOffset The offset starting from the first descriptor in the D3D12_ROOT_DESCRIPTOR_TABLE. This must refer to a descriptor in a D3D12_DESCRIPTOR_RANGE of type D3D12_DESCRIPTOR_RANGE_TYPE_UAV.
		* @param resource The resource to bind.
		* @param stateAfter The required state of the resource.
		* @param firstSubresource The first subresource to transition to the requested state.
		* @param numSubresources The number of subresources to transition.
		* @param uav The UAV description to use for the resource in the shader.
		*/
		void SetUnorderedAccessView(
			uint32 rootParameterIndex,
			uint32 descriptorOffset,
			const std::shared_ptr<Resource>& pResource,
			D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			UINT firstSubresource = 0,
			UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav = nullptr);

		void SetRenderTarget(const RenderTarget& renderTarget);

		void Draw(uint32 vertexCount, uint32 instanceCount, uint32 startVertex, uint32 startInstance);

		Microsoft::WRL::ComPtr<DX12_COMMAND_LIST_TYPE> GetGraphicsCommandList() const { return m_d3d12CommandList; }

	private:
		Microsoft::WRL::ComPtr<DX12_COMMAND_LIST_TYPE> m_d3d12CommandList;
		D3D12_COMMAND_LIST_TYPE m_CommandListType;

		std::unique_ptr<ResourceStateTracker> m_pResourceStateTracker;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Object>> m_TrackedObjects;

		std::unique_ptr<UploadBuffer> m_pUploadBuffer;
		std::unique_ptr<DynamicDescriptorHeap> m_pDynamicDescriptorHeap[2]; // CBV_SRV_UAV and SAMPER

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_d3d12CommandAllocator;
	};
}
#pragma once

#include "DX12/Dx12Device.h"

#include "DX12/NewCore/Resource.h"
#include "DX12/NewCore/ResourceStateTracker.h"
#include "DX12/NewCore/DynamicDescriptorHeap.h"
#include "DX12/NewCore/RenderTarget.h"
#include "DX12/NewCore/RootSignature.h"
#include "DX12/NewCore/UploadBuffer.h"

#define DX12_COMMAND_LIST_TYPE ID3D12GraphicsCommandList2

namespace RS
{
	// This is more like a RenderContext.
	// One for each thread.
	class CommandList
	{
	public:
		explicit CommandList(D3D12_COMMAND_LIST_TYPE type);
		virtual ~CommandList();

		void Reset();

		bool Close(CommandList& pendingCommandList);
		// Just close the command list. This is usefull for pending command lists.
		void Close();

		void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);

		void ClearTexture(const std::shared_ptr<Texture>& pTexture, const float clearColor[4]);
		void ClearTextures(const std::vector<std::shared_ptr<Texture>>& textures, const float clearColor[4]);
		void ClearDSV(const std::shared_ptr<Texture>& pTexture, D3D12_CLEAR_FLAGS clearFlags, float depth, uint8 stencil);

		std::shared_ptr<Buffer> CreateBufferResource(uint64 size, const std::string& name);
		std::shared_ptr<Buffer> CreateBufferResource(uint64 size, const D3D12_CLEAR_VALUE* pClearValue, const std::string& name);
		std::shared_ptr<VertexBuffer> CreateVertexBufferResource(uint64 size, uint32 stride, const std::string& name);
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(size_t bufferSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags);

		std::shared_ptr<Texture> CreateTexture(uint32 width, uint32 height, const uint8* pPixelData, DXGI_FORMAT format, const std::string& name, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, D3D12_CLEAR_VALUE* pClearValue = nullptr);

		void UploadToBuffer(std::shared_ptr<Buffer> pBuffer, size_t bufferSize, const void* bufferData);
		void UploadTextureSubresourceData(const std::shared_ptr<Texture>& pTexture, uint32 firstSubresource, uint32 numSubresources, D3D12_SUBRESOURCE_DATA* pSubresourceData);

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

		void SetViewport(const D3D12_VIEWPORT& viewport);
		void SetViewports(const std::vector<D3D12_VIEWPORT>& viewports);
		void SetScissorRect(const D3D12_RECT scissorRect);
		void SetScissorRects(const std::vector<D3D12_RECT>& scissorRects);

		void SetPipelineState(ID3D12PipelineState* pPipelineState, bool forceUpdate = false);
		void SetRootSignature(const std::shared_ptr<RootSignature>& pRootSignature);
		void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* pHeap);
		void BindDescriptorHeaps();

		void SetGraphicsDynamicConstantBuffer(uint32 rootParameterIndex, size_t sizeInBytes, const void* bufferData);

		void SetGraphicsRoot32BitConstants(uint32 rootParameterIndex, uint32 numConstants, const void* pConstants);

		void SetVertexBuffers(uint32 slot, const std::shared_ptr<VertexBuffer>& pVertexBuffer);
		void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& pIndexBuffer);

		void SetBlendFactor(const float blend_factor[4]);

		/** Binds a resource as an SRV.
		* @param rootParameterIndex The root parameter index to assign the SRV to. The root parameter must be of type D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE.
		* @param descriptorOffset The offset starting from the first descriptor in the D3D12_ROOT_DESCRIPTOR_TABLE. This must refer to a descriptor in a D3D12_DESCRIPTOR_RANGE of type D3D12_DESCRIPTOR_RANGE_TYPE_SRV.
		* @param resource The resource to bind.
		* @param stateAfter The required state of the resource.
		* @param firstSubresource The first subresource to transition to the requested state.
		* @param numSubresources The number of subresources to transition.
		* @param srv The SRV description to use for the resource in the shader.
		*/
		void BindShaderResourceView(
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
		void BindUnorderedAccessView(
			uint32 rootParameterIndex,
			uint32 descriptorOffset,
			const std::shared_ptr<Resource>& pResource,
			D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
			UINT firstSubresource = 0,
			UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav = nullptr);

		void BindBuffer(uint32 rootParameterIndex, uint32 descriptorOffset, const std::shared_ptr<Buffer>& pTexture,
			D3D12_SHADER_RESOURCE_VIEW_DESC* pSRVDesc = nullptr,
			D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		void BindTexture(uint32 rootParameterIndex, uint32 descriptorOffset, const std::shared_ptr<Texture>& pTexture,
			D3D12_SHADER_RESOURCE_VIEW_DESC* pSRVDesc = nullptr,
			D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		// Same ass the above except that it will increment the offset for each invocation before the reset.
		void BindTexture(uint32 rootParameterIndex, const std::shared_ptr<Texture>& pTexture,
			D3D12_SHADER_RESOURCE_VIEW_DESC* pSRVDesc = nullptr,
			D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		enum class RenderTargetMode : uint
		{
			All = 0,
			DepthOnly,
			ColorOnly
		};
		void SetRenderTarget(const std::shared_ptr<RenderTarget>& pRenderTarget, RenderTargetMode mode = RenderTargetMode::All);

		void DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 startVertex, uint32 startInstance);
		void DrawIndexInstanced(uint32 indexCountPerInstance, uint32 instanceCount, uint32 startIndexLocation, int32 baseVertexLocation, uint32 startInstanceLocation);

		void ResizeTexture(const std::shared_ptr<Texture>& pTexture, uint32 newWidth, uint32 newHeight);

		Microsoft::WRL::ComPtr<DX12_COMMAND_LIST_TYPE> GetGraphicsCommandList() const { return m_d3d12CommandList; }

		RenderTarget* GetCurrentRenderTarget();

	private:
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_d3d12CommandAllocator;
		Microsoft::WRL::ComPtr<DX12_COMMAND_LIST_TYPE> m_d3d12CommandList;
		D3D12_COMMAND_LIST_TYPE m_CommandListType;

		std::unique_ptr<ResourceStateTracker> m_pResourceStateTracker;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Object>> m_TrackedObjects;

		std::unique_ptr<UploadBuffer> m_pUploadBuffer;
		std::unique_ptr<DynamicDescriptorHeap> m_pDynamicDescriptorHeap[2]; // CBV_SRV_UAV and SAMPER
		uint32 m_pDynamicDescriptorOffsets[2]; // CBV_SRV_UAV and SAMPER
		ID3D12DescriptorHeap* m_pDescriptorHeaps[2];

		ID3D12RootSignature* m_pRootSignature;
		ID3D12PipelineState* m_pPipelineState;

		RenderTarget* m_pCurrentRenderTarget = nullptr;

		uint64 m_ID;
	};
}
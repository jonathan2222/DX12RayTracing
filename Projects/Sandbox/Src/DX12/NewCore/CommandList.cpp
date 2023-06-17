#include "PreCompiled.h"
#include "CommandList.h"

#include "Core/EngineLoop.h"
#include "DX12/NewCore/DX12Core3.h"

RS::CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator)
    : m_CommandListType(type)
    , m_d3d12CommandAllocator(allocator)
    , m_pRootSignature(nullptr)
    , m_pPipelineState(nullptr)
{
    auto pDevice = DX12Core3::Get()->GetD3D12Device();
    DXCallVerbose(pDevice->CreateCommandList(0, m_CommandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&m_d3d12CommandList)), "Failed to create the command list!");

    for (uint32 i = 0; i < 2; ++i)
    {
        m_pDynamicDescriptorHeap[i] = std::make_unique<DynamicDescriptorHeap>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
        m_pDescriptorHeaps[i] = nullptr;
    }

    m_pUploadBuffer = std::make_unique<RS::UploadBuffer>();
    m_pResourceStateTracker = std::make_unique<ResourceStateTracker>();
}

RS::CommandList::~CommandList()
{
}

void RS::CommandList::Reset()
{
    DXCall(m_d3d12CommandAllocator->Reset());
    DXCall(m_d3d12CommandList->Reset(m_d3d12CommandAllocator.Get(), nullptr));

    m_pResourceStateTracker->Reset();
    m_pUploadBuffer->Reset();

    ReleaseTrackedObjects();

    for (uint32 i = 0; i < 2; ++i)
    {
        m_pDynamicDescriptorHeap[i]->Reset();
        m_pDescriptorHeaps[i] = nullptr;
    }

    m_pRootSignature = nullptr;
    m_pPipelineState = nullptr;
}

void RS::CommandList::Close()
{
    FlushResourceBarriers();
    m_d3d12CommandList->Close();
}

void RS::CommandList::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
{
    m_d3d12CommandList->IASetPrimitiveTopology(primitiveTopology);
}

void RS::CommandList::ClearTexture(const std::shared_ptr<Texture>& pTexture, const float clearColor[4])
{
    RS_ASSERT_NO_MSG(pTexture);

    TransitionBarrier(pTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
    m_d3d12CommandList->ClearRenderTargetView(pTexture->GetRenderTargetView(), clearColor, 0, nullptr);

    TrackResource(pTexture);
}

void RS::CommandList::ClearDSV(const std::shared_ptr<Texture>& pTexture, D3D12_CLEAR_FLAGS clearFlags, float depth, uint8 stencil)
{
    RS_ASSERT_NO_MSG(pTexture);

    TransitionBarrier(pTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
    m_d3d12CommandList->ClearDepthStencilView(pTexture->GetRenderTargetView(), clearFlags, depth, stencil, 0, nullptr);

    TrackResource(pTexture);
}

std::shared_ptr<RS::Buffer > RS::CommandList::CreateBufferResource(uint64 size, const std::string& name)
{
    return CreateBufferResource(size, nullptr, name);
}

std::shared_ptr<RS::Buffer> RS::CommandList::CreateBufferResource(uint64 size, const D3D12_CLEAR_VALUE* pClearValue, const std::string& name)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE; // This can be different. TODO: Add to arguments.
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; // This can also be different.
    desc.Height = desc.DepthOrArraySize = desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    std::shared_ptr<Buffer> pBuffer = std::make_shared<Buffer>(desc, pClearValue, name);
    pBuffer->m_DescriptorAllocation = DX12Core3::Get()->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    return pBuffer;
}

std::shared_ptr<RS::VertexBuffer> RS::CommandList::CreateVertexBufferResource(uint64 size, uint32 stride, const std::string& name)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE; // This can be different. TODO: Add to arguments.
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; // This can also be different.
    desc.Height = desc.DepthOrArraySize = desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    std::shared_ptr<VertexBuffer> pVertexBuffer = std::make_shared<VertexBuffer>(stride, desc, nullptr, name);
    // Might not need this, if we are not going to read/write this data in a shader.
    pVertexBuffer->m_DescriptorAllocation = DX12Core3::Get()->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    return pVertexBuffer;
}

void RS::CommandList::TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, D3D12_RESOURCE_STATES stateAfter, UINT subResource, bool flushBarriers)
{
    if (pResource)
    {
        // The "before" state is not important. It will be resolved by the resource state tracker.
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pResource.Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter, subResource);
        m_pResourceStateTracker->ResourceBarrier(barrier);
    }

    if (flushBarriers)
        FlushResourceBarriers();
}

void RS::CommandList::TransitionBarrier(const std::shared_ptr<Resource>& pResource, D3D12_RESOURCE_STATES stateAfter, UINT subResource, bool flushBarriers)
{
    TransitionBarrier(pResource->GetD3D12Resource(), stateAfter, subResource, flushBarriers);
}

void RS::CommandList::UAVBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, bool flushBarriers)
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(pResource.Get());
    m_pResourceStateTracker->ResourceBarrier(barrier);

    if (flushBarriers)
        FlushResourceBarriers();
}

void RS::CommandList::UAVBarrier(const std::shared_ptr<Resource>& pResource, bool flushBarriers)
{
    auto d3d12Resource = pResource ? pResource->GetD3D12Resource() : nullptr;
    UAVBarrier(d3d12Resource, flushBarriers);
}

void RS::CommandList::AliasingBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> pResourceBefore, Microsoft::WRL::ComPtr<ID3D12Resource> pResourceAfter, bool flushBarriers)
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(pResourceBefore.Get(), pResourceAfter.Get());
    m_pResourceStateTracker->ResourceBarrier(barrier);

    if (flushBarriers)
        FlushResourceBarriers();
}

void RS::CommandList::AliasingBarrier(const std::shared_ptr<Resource>& pResourceBefore, const std::shared_ptr<Resource>& pResourceAfter, bool flushBarriers)
{
    auto d3d12ResourceBefore = pResourceBefore ? pResourceBefore->GetD3D12Resource() : nullptr;
    auto d3d12ResourceAfter = pResourceAfter ? pResourceAfter->GetD3D12Resource() : nullptr;
    AliasingBarrier(d3d12ResourceBefore, d3d12ResourceAfter, flushBarriers);
}

void RS::CommandList::FlushResourceBarriers()
{
    m_pResourceStateTracker->FlushResourceBarriers(*this);
}

void RS::CommandList::TrackResource(Microsoft::WRL::ComPtr<ID3D12Object> object)
{
    m_TrackedObjects.push_back(object);
}

void RS::CommandList::TrackResource(const std::shared_ptr<Resource>& resource)
{
    RS_ASSERT_NO_MSG(resource);
    TrackResource(resource->GetD3D12Resource());
}

void RS::CommandList::ReleaseTrackedObjects()
{
    m_TrackedObjects.clear();
}

void RS::CommandList::CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> dstResource, Microsoft::WRL::ComPtr<ID3D12Resource> srcResource)
{
    TransitionBarrier(dstResource, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionBarrier(srcResource, D3D12_RESOURCE_STATE_COPY_SOURCE);

    FlushResourceBarriers();

    m_d3d12CommandList->CopyResource(dstResource.Get(), srcResource.Get());

    TrackResource(dstResource);
    TrackResource(srcResource);
}

void RS::CommandList::CopyResource(const std::shared_ptr<Resource>& dstResource, const std::shared_ptr<Resource>& srcResource)
{
    CopyResource(dstResource->GetD3D12Resource(), srcResource->GetD3D12Resource());
}

Microsoft::WRL::ComPtr<ID3D12Resource> RS::CommandList::CreateBuffer(size_t bufferSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
    ComPtr<ID3D12Resource> d3d12Resource;
    if (bufferSize == 0)
    {
        // This will result in a NULL resource (which may be desired to define a default null resource).
        return d3d12Resource;
    }

    auto pDevice = DX12Core3::Get()->GetD3D12Device();
    DXCall(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags), D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_PPV_ARGS(&d3d12Resource)));

    ResourceStateTracker::AddGlobalResourceState(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

    if (bufferData != nullptr)
    {
        // Create an upload resource to use as an intermediate buffer to copy the buffer resource.
        ComPtr<ID3D12Resource> uploadResource;
        DXCall(pDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&uploadResource)));

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = bufferData;
        subresourceData.RowPitch = bufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        m_pResourceStateTracker->TransitionResource(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();

        UpdateSubresources(m_d3d12CommandList.Get(), d3d12Resource.Get(), uploadResource.Get(), 0, 0, 1,
            &subresourceData);

        // Add references to resources so they stay in scope until the command list is reset.
        TrackResource(uploadResource);
    }
    TrackResource(d3d12Resource);

    return d3d12Resource;
}

void RS::CommandList::UploadToBuffer(std::shared_ptr<Buffer> pBuffer, size_t bufferSize, const void* bufferData)
{
    if (bufferData != nullptr)
    {
        auto pDevice = DX12Core3::Get()->GetD3D12Device();

        // Create an upload resource to use as an intermediate buffer to copy the buffer resource.
        ComPtr<ID3D12Resource> pUploadResource;
        DXCall(pDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&pUploadResource)));

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = bufferData;
        subresourceData.RowPitch = bufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        m_pResourceStateTracker->TransitionResource(pBuffer->GetD3D12Resource().Get(), D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();

        UpdateSubresources(m_d3d12CommandList.Get(), pBuffer->GetD3D12Resource().Get(), pUploadResource.Get(), 0, 0, 1,
            &subresourceData);

        // Add references to resources so they stay in scope until the command list is reset.
        TrackResource(pUploadResource);
        TrackResource(pBuffer);
    }
}

void RS::CommandList::SetViewport(const D3D12_VIEWPORT& viewport)
{
    SetViewports({ viewport });
}

void RS::CommandList::SetViewports(const std::vector<D3D12_VIEWPORT>& viewports)
{
    RS_ASSERT_NO_MSG(viewports.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
    m_d3d12CommandList->RSSetViewports(static_cast<UINT>(viewports.size()), viewports.data());
}

void RS::CommandList::SetScissorRects(const D3D12_RECT scissorRect)
{
    SetScissorRects({ scissorRect });
}

void RS::CommandList::SetScissorRects(const std::vector<D3D12_RECT>& scissorRects)
{
    RS_ASSERT_NO_MSG(scissorRects.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
    m_d3d12CommandList->RSSetScissorRects(static_cast<UINT>(scissorRects.size()), scissorRects.data());
}

void RS::CommandList::SetPipelineState(ID3D12PipelineState* pPipelineState)
{
    RS_ASSERT_NO_MSG(pPipelineState);

    if (m_pPipelineState != pPipelineState)
    {
        m_pPipelineState = pPipelineState;

        m_d3d12CommandList->SetPipelineState(pPipelineState);

        TrackResource(pPipelineState);
    }
}

void RS::CommandList::SetRootSignature(const std::shared_ptr<RootSignature>& pRootSignature)
{
    RS_ASSERT_NO_MSG(pRootSignature);

    auto d3d12RootSignature = pRootSignature->GetRootSignature().Get();
    if (m_pRootSignature != d3d12RootSignature)
    {
        m_pRootSignature = d3d12RootSignature;

        for (uint32 i = 0; i < 2; ++i)
            m_pDynamicDescriptorHeap[i]->ParseRootSignature(pRootSignature);

        m_d3d12CommandList->SetGraphicsRootSignature(m_pRootSignature);

        TrackResource(m_pRootSignature);
    }
}

void RS::CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* pHeap)
{
    if (m_pDescriptorHeaps[type] != pHeap)
    {
        m_pDescriptorHeaps[type] = pHeap;
        BindDescriptorHeaps();
    }
}

void RS::CommandList::BindDescriptorHeaps()
{
    UINT numDescriptorHeaps = 0;
    ID3D12DescriptorHeap* pDescriptorHeaps[2] = {};

    for (uint32 i = 0; i < 2; ++i)
    {
        ID3D12DescriptorHeap* pDescriptorHeap = m_pDescriptorHeaps[i];
        if (pDescriptorHeap)
        {
            pDescriptorHeaps[numDescriptorHeaps++] = pDescriptorHeap;
        }
    }

    m_d3d12CommandList->SetDescriptorHeaps(numDescriptorHeaps, pDescriptorHeaps);
}

void RS::CommandList::SetGraphicsDynamicConstantBuffer(uint32 rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
    // Constant buffers must be 256-byte aligned.
    auto heapAllococation = m_pUploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(heapAllococation.CPU, bufferData, sizeInBytes);

    m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllococation.GPU);
}

void RS::CommandList::SetGraphicsRoot32BitConstants(uint32 rootParameterIndex, uint32 numConstants, const void* pConstants)
{
    m_d3d12CommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, numConstants, pConstants, 0);
}

void RS::CommandList::SetVertexBuffers(uint32 slot, const std::shared_ptr<VertexBuffer>& pVertexBuffer)
{
    D3D12_VERTEX_BUFFER_VIEW view = pVertexBuffer->CreateView();
    m_d3d12CommandList->IASetVertexBuffers(slot, 1, &view);
}

void RS::CommandList::SetShaderResourceView(uint32 rootParameterIndex, uint32 descriptorOffset, const std::shared_ptr<Resource>& pResource, D3D12_RESOURCE_STATES stateAfter, UINT firstSubresource, UINT numSubresources, const D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
{
    if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        for (uint32_t i = 0; i < numSubresources; ++i)
        {
            TransitionBarrier(pResource, stateAfter, firstSubresource + i);
        }
    }
    else
    {
        TransitionBarrier(pResource, stateAfter);
    }

    // TODO: Option to create the shader resource view somewhere else and not do it all the time.
    m_pDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, pResource.get()->GetShaderResourceView(srv));

    TrackResource(pResource);
}

void RS::CommandList::SetUnorderedAccessView(uint32 rootParameterIndex, uint32 descriptorOffset, const std::shared_ptr<Resource>& pResource, D3D12_RESOURCE_STATES stateAfter, UINT firstSubresource, UINT numSubresources, const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav)
{
    if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        for (uint32_t i = 0; i < numSubresources; ++i)
        {
            TransitionBarrier(pResource, stateAfter, firstSubresource + i);
        }
    }
    else
    {
        TransitionBarrier(pResource, stateAfter);
    }

    // TODO: Option to create the unordered access view somewhere else and not do it all the time.
    m_pDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, pResource.get()->GetUnorderedAccessView(uav));

    TrackResource(pResource);
}

void RS::CommandList::SetRenderTarget(const RenderTarget& renderTarget)
{
    std::vector<std::shared_ptr<Texture>>& colorTextures = renderTarget.GetColorTextures();

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetDescriptors;

    // Max 8 render targets can be bound to the rendering pipeline.
    for (uint32 i = 0; i < 8; ++i)
    {
        std::shared_ptr<Texture> pTexture = colorTextures[i];
        if (pTexture)
        {
            TransitionBarrier(pTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
            renderTargetDescriptors.push_back(pTexture->GetRenderTargetView());

            // Track it such that it will not get lost while in-flight (used for rendering).
            TrackResource(pTexture);
        }
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor(D3D12_DEFAULT);
    std::shared_ptr<Texture> pDepthTexture = renderTarget.GetDepthTextures();
    if (pDepthTexture)
    {
        TransitionBarrier(pDepthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        depthStencilDescriptor = pDepthTexture->GetDepthStencilView();

        // Track it such that it will not get lost while in-flight (used for rendering).
        TrackResource(pDepthTexture);
    }
    D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = depthStencilDescriptor.ptr != 0 ? &depthStencilDescriptor : nullptr;
    m_d3d12CommandList->OMSetRenderTargets(renderTargetDescriptors.size(), renderTargetDescriptors.data(), FALSE, pDSV);
}

void RS::CommandList::DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 startVertex, uint32 startInstance)
{
    FlushResourceBarriers();

    for (int i = 0; i < 2; ++i)
        m_pDynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this); // Binds the descriptors

    m_d3d12CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

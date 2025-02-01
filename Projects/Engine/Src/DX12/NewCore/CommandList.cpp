#include "PreCompiled.h"
#include "CommandList.h"

#include "Core/EngineLoop.h"
#include "DX12/NewCore/DX12Core3.h"

RS::CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type)
    : m_CommandListType(type)
    , m_pRootSignature(nullptr)
    , m_pPipelineState(nullptr)
{
    auto pDevice = DX12Core3::Get()->GetD3D12Device();
    DXCallVerbose(pDevice->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&m_d3d12CommandAllocator)));
    DXCallVerbose(pDevice->CreateCommandList(0, m_CommandListType, m_d3d12CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_d3d12CommandList)), "Failed to create the command list!");

    for (uint32 i = 0; i < 2; ++i)
    {
        m_pDynamicDescriptorHeap[i] = std::make_unique<DynamicDescriptorHeap>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
        m_pDescriptorHeaps[i] = nullptr;
        m_pDynamicDescriptorOffsets[i] = 0;
    }

    m_pUploadBuffer = std::make_unique<RS::UploadBuffer>();
    m_pResourceStateTracker = std::make_unique<ResourceStateTracker>();

    static uint64 s_IDGenerator = 0;
    m_ID = s_IDGenerator++;
}

RS::CommandList::~CommandList()
{}

void RS::CommandList::Reset()
{
    DXCallVerbose(m_d3d12CommandAllocator->Reset());
    DXCallVerbose(m_d3d12CommandList->Reset(m_d3d12CommandAllocator.Get(), nullptr));

    m_pResourceStateTracker->Reset();
    m_pUploadBuffer->Reset();

    ReleaseTrackedObjects();

    for (uint32 i = 0; i < 2; ++i)
    {
        m_pDynamicDescriptorHeap[i]->Reset();
        m_pDescriptorHeaps[i] = nullptr;
        m_pDynamicDescriptorOffsets[i] = 0;
    }

    m_pRootSignature = nullptr;
    m_pPipelineState = nullptr;
}

bool RS::CommandList::Close(CommandList& pendingCommandList)
{
    // Flush any remaining barriers.
    FlushResourceBarriers();

    m_d3d12CommandList->Close();

    // Flush pending resource barriers.
    uint32 numPendingBarriers = m_pResourceStateTracker->FlushPendingResourceBarriers(pendingCommandList);
    // Commit the final resource state to the global state.
    m_pResourceStateTracker->CommitFinalResourceStates();

    return numPendingBarriers > 0;
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
    RS_ASSERT(pTexture);

    TransitionBarrier(pTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
    m_d3d12CommandList->ClearRenderTargetView(pTexture->GetRenderTargetView(), clearColor, 0, nullptr);

    TrackResource(pTexture);
}

void RS::CommandList::ClearTextures(const std::vector<std::shared_ptr<Texture>>& textures, const float clearColor[4])
{
    for (auto& pTexture : textures)
        ClearTexture(pTexture, clearColor);
}

void RS::CommandList::ClearDSV(const std::shared_ptr<Texture>& pTexture, D3D12_CLEAR_FLAGS clearFlags, float depth, uint8 stencil)
{
    RS_ASSERT(pTexture);

    TransitionBarrier(pTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
    m_d3d12CommandList->ClearDepthStencilView(pTexture->GetDepthStencilView(), clearFlags, depth, stencil, 0, nullptr);

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

    return std::make_shared<Buffer>(desc, pClearValue, name);
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

    return std::make_shared<VertexBuffer>(stride, desc, nullptr, name);
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
    RS_ASSERT(resource);
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

void RS::CommandList::UploadTextureSubresourceData(const std::shared_ptr<Texture>& pTexture, uint32 firstSubresource, uint32 numSubresources, D3D12_SUBRESOURCE_DATA* pSubresourceData)
{
    auto pDevice = DX12Core3::Get()->GetD3D12Device();
    auto pDestinationResource = pTexture->GetD3D12Resource();

    if (pDestinationResource)
    {
        TransitionBarrier(pTexture, D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();

        UINT64 requiredSize = GetRequiredIntermediateSize(pDestinationResource.Get(), firstSubresource, numSubresources);

        // Create a temporary (intermediate) resource for uploading the subresoruce
        ComPtr<ID3D12Resource> pIntermediateResrouce;
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);
        DXCall(pDevice->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE,
            &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&pIntermediateResrouce)));

        UpdateSubresources(m_d3d12CommandList.Get(), pDestinationResource.Get(), pIntermediateResrouce.Get(), 0,
            firstSubresource, numSubresources, pSubresourceData);

        TrackResource(pIntermediateResrouce);
        TrackResource(pDestinationResource);
    }
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
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
    DXCall(pDevice->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_PPV_ARGS(&d3d12Resource)));

    ResourceStateTracker::AddGlobalResourceState(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

    if (bufferData != nullptr)
    {
        // Create an upload resource to use as an intermediate buffer to copy the buffer resource.
        ComPtr<ID3D12Resource> uploadResource;
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        DXCall(pDevice->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE,
            &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
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

std::shared_ptr<RS::Texture> RS::CommandList::CreateTexture(uint32 width, uint32 height, const uint8* pPixelData, DXGI_FORMAT format, const std::string& name, D3D12_RESOURCE_FLAGS flags, D3D12_CLEAR_VALUE* pClearValue)
{
    RS_ASSERT(width != 0 && height != 0);

    D3D12_RESOURCE_DESC textureDesc{};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Alignment = 0;
    textureDesc.Width = (UINT64)width;
    textureDesc.Height = height;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = flags;

    auto pDevice = DX12Core3::Get()->GetD3D12Device();

    ComPtr<ID3D12Resource> pResource;
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    DXCall(pDevice->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &textureDesc, D3D12_RESOURCE_STATE_COMMON, pClearValue,
        IID_PPV_ARGS(&pResource)));

    std::shared_ptr<Texture> pTexture = std::make_shared<Texture>(pResource, name);

    if (pClearValue)
    {
        RS_ASSERT(pTexture->m_pD3D12ClearValue == nullptr);
        pTexture->m_pD3D12ClearValue = new D3D12_CLEAR_VALUE();
        memcpy(pTexture->m_pD3D12ClearValue, pClearValue, sizeof(D3D12_CLEAR_VALUE));
    }

    ResourceStateTracker::AddGlobalResourceState(pResource.Get(), D3D12_RESOURCE_STATE_COMMON);

    if (pPixelData)
    {
        uint32 numChannels = DX12::GetChannelCountFromFormat(format);

        // Copy data to the intermediate upload heap and then schedule a copy from the upload heap to the texture resource.
        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = pPixelData;
        textureData.RowPitch = static_cast<LONG_PTR>(numChannels * textureDesc.Width);
        textureData.SlicePitch = textureData.RowPitch * textureDesc.Height;

        UploadTextureSubresourceData(pTexture, 0, 1, &textureData);
    }

    return pTexture;
}

void RS::CommandList::UploadToBuffer(std::shared_ptr<Buffer> pBuffer, size_t bufferSize, const void* bufferData)
{
    if (bufferData != nullptr)
    {
        auto pDevice = DX12Core3::Get()->GetD3D12Device();

        // Create an upload resource to use as an intermediate buffer to copy the buffer resource.
        ComPtr<ID3D12Resource> pUploadResource;
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        DXCall(pDevice->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE,
            &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
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
    RS_ASSERT(viewports.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
    m_d3d12CommandList->RSSetViewports(static_cast<UINT>(viewports.size()), viewports.data());
}

void RS::CommandList::SetScissorRect(const D3D12_RECT scissorRect)
{
    SetScissorRects({ scissorRect });
}

void RS::CommandList::SetScissorRects(const std::vector<D3D12_RECT>& scissorRects)
{
    RS_ASSERT(scissorRects.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
    m_d3d12CommandList->RSSetScissorRects(static_cast<UINT>(scissorRects.size()), scissorRects.data());
}

void RS::CommandList::SetPipelineState(ID3D12PipelineState* pPipelineState, bool forceUpdate)
{
    RS_ASSERT(pPipelineState);

    if (m_pPipelineState != pPipelineState || forceUpdate)
    {
        m_pPipelineState = pPipelineState;

        m_d3d12CommandList->SetPipelineState(pPipelineState);

        TrackResource(pPipelineState);
    }
}

void RS::CommandList::SetRootSignature(const std::shared_ptr<RootSignature>& pRootSignature)
{
    RS_ASSERT(pRootSignature);

    auto d3d12RootSignature = pRootSignature->GetRootSignature().Get();
    if (m_pRootSignature != d3d12RootSignature)
    {
        m_pRootSignature = d3d12RootSignature;

        for (uint32 i = 0; i < 2; ++i)
        {
            m_pDynamicDescriptorHeap[i]->ParseRootSignature(pRootSignature);
            m_pDynamicDescriptorOffsets[i] = 0;
        }

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
    RS_ASSERT(bufferData);

    // Constant buffers must be 256-byte aligned.
    auto heapAllococation = m_pUploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(heapAllococation.CPU, bufferData, sizeInBytes);

    m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllococation.GPU);
}

void RS::CommandList::SetGraphicsRoot32BitConstants(uint32 rootParameterIndex, uint32 numConstants, const void* pConstants)
{
    RS_ASSERT(pConstants);

    m_d3d12CommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, numConstants, pConstants, 0);
}

void RS::CommandList::SetVertexBuffers(uint32 slot, const std::shared_ptr<VertexBuffer>& pVertexBuffer)
{
    RS_ASSERT(pVertexBuffer);

    D3D12_VERTEX_BUFFER_VIEW view = pVertexBuffer->CreateView();
    m_d3d12CommandList->IASetVertexBuffers(slot, 1, &view);
}

void RS::CommandList::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& pIndexBuffer)
{
    RS_ASSERT(pIndexBuffer);

    D3D12_INDEX_BUFFER_VIEW view = pIndexBuffer->CreateView();
    m_d3d12CommandList->IASetIndexBuffer(&view);
}

void RS::CommandList::SetBlendFactor(const float blendFactor[4])
{
    RS_ASSERT(blendFactor);

    m_d3d12CommandList->OMSetBlendFactor(blendFactor);
}

void RS::CommandList::BindShaderResourceView(uint32 rootParameterIndex, uint32 descriptorOffset, const std::shared_ptr<Resource>& pResource, D3D12_RESOURCE_STATES stateAfter, UINT firstSubresource, UINT numSubresources, const D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
{
    RS_ASSERT(pResource);

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

    m_pDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, pResource.get()->GetShaderResourceView(srv));

    TrackResource(pResource);
}

void RS::CommandList::BindUnorderedAccessView(uint32 rootParameterIndex, uint32 descriptorOffset, const std::shared_ptr<Resource>& pResource, D3D12_RESOURCE_STATES stateAfter, UINT firstSubresource, UINT numSubresources, const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav)
{
    RS_ASSERT(pResource);

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

    m_pDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, pResource.get()->GetUnorderedAccessView(uav));

    TrackResource(pResource);
}

void RS::CommandList::BindBuffer(uint32 rootParameterIndex, uint32 descriptorOffset, const std::shared_ptr<Buffer>& pBuffer,
    D3D12_SHADER_RESOURCE_VIEW_DESC* pSRVDesc, D3D12_RESOURCE_STATES stateAfter)
{
    RS_ASSERT(pBuffer);

    TransitionBarrier(pBuffer, stateAfter);

    m_pDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, pBuffer->GetShaderResourceView(pSRVDesc));

    TrackResource(pBuffer);
}

void RS::CommandList::BindTexture(uint32 rootParameterIndex, uint32 descriptorOffset, const std::shared_ptr<Texture>& pTexture,
    D3D12_SHADER_RESOURCE_VIEW_DESC* pSRVDesc, D3D12_RESOURCE_STATES stateAfter)
{
    RS_ASSERT(pTexture);

    TransitionBarrier(pTexture, stateAfter);

    m_pDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, pTexture->GetShaderResourceView(pSRVDesc));

    TrackResource(pTexture);
}

void RS::CommandList::BindTexture(uint32 rootParameterIndex, const std::shared_ptr<Texture>& pTexture,
    D3D12_SHADER_RESOURCE_VIEW_DESC* pSRVDesc, D3D12_RESOURCE_STATES stateAfter)
{
    RS_ASSERT(pTexture);

    TransitionBarrier(pTexture, stateAfter);

    uint32& descriptorOffset = m_pDynamicDescriptorOffsets[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];

    m_pDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, pTexture->GetShaderResourceView(pSRVDesc));

    // Note: this migth cause issues. If it does not work use the other BindTexture function and specify the offset manually.
    descriptorOffset++;

    TrackResource(pTexture);
}

void RS::CommandList::SetRenderTarget(const std::shared_ptr<RenderTarget>& pRenderTarget, RenderTargetMode mode)
{
    const std::vector<std::shared_ptr<Texture>>& colorTextures = pRenderTarget->GetColorTextures();

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetDescriptors;

    // Max 8 render targets can be bound to the rendering pipeline.
    if (mode == RenderTargetMode::All || mode == RenderTargetMode::ColorOnly)
    {
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
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor(D3D12_DEFAULT);
    if (mode == RenderTargetMode::All || mode == RenderTargetMode::DepthOnly)
    {
        std::shared_ptr<Texture> pDepthTexture = pRenderTarget->GetDepthTexture();
        if (pDepthTexture)
        {
            TransitionBarrier(pDepthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            depthStencilDescriptor = pDepthTexture->GetDepthStencilView();

            // Track it such that it will not get lost while in-flight (used for rendering).
            TrackResource(pDepthTexture);
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = depthStencilDescriptor.ptr != 0 ? &depthStencilDescriptor : nullptr;
    m_d3d12CommandList->OMSetRenderTargets(renderTargetDescriptors.size(), renderTargetDescriptors.data(), FALSE, pDSV);

    m_pCurrentRenderTarget = pRenderTarget.get();
}

void RS::CommandList::DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 startVertex, uint32 startInstance)
{
    FlushResourceBarriers();

    for (int i = 0; i < 2; ++i)
        m_pDynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this); // Binds the descriptors

    m_d3d12CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void RS::CommandList::DrawIndexInstanced(uint32 indexCountPerInstance, uint32 instanceCount, uint32 startIndexLocation, int32 baseVertexLocation, uint32 startInstanceLocation)
{
    FlushResourceBarriers();

    for (int i = 0; i < 2; ++i)
        m_pDynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this); // Binds the descriptors

    m_d3d12CommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

RS::RenderTarget* RS::CommandList::GetCurrentRenderTarget()
{
    return m_pCurrentRenderTarget;
}

#include "PreCompiled.h"
#include "CommandList.h"

#include "Core/EngineLoop.h"
#include "DX12/NewCore/DX12Core3.h"

RS::CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator)
    : m_CommandListType(type)
    , m_d3d12CommandAllocator(allocator)
{
    auto pDevice = DX12Core3::Get()->GetD3D12Device();
    DXCallVerbose(pDevice->CreateCommandList(0, m_CommandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&m_d3d12CommandList)), "Failed to create the command list!");
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

    m_pDynamicDescriptorHeap[0]->Reset();
    m_pDynamicDescriptorHeap[1]->Reset();
}

void RS::CommandList::Close()
{
    FlushResourceBarriers();
    m_d3d12CommandList->Close();
}

RS::Resource* RS::CommandList::CreateBufferResource()
{
    Resource* pResource = new Resource();
    DescriptorAllocator* pDescriptorAllocator = DX12Core3::Get()->GetShaderResourceDescriptorAllocator();
    pResource->m_DescriptorAllocation = pDescriptorAllocator->Allocate();
    pResource->m_pD3D12Resource = nullptr; // TODO: Set it to the buffer.
    return pResource;
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

Microsoft::WRL::ComPtr<ID3D12Resource> RS::CommandList::CopyBuffer(size_t bufferSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
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

void RS::CommandList::SetGraphicsDynamicConstantBuffer(uint32 rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
    // Constant buffers must be 256-byte aligned.
    auto heapAllococation = m_pUploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(heapAllococation.CPU, bufferData, sizeInBytes);

    m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllococation.GPU);
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
}

void RS::CommandList::Draw(uint32 vertexCount, uint32 instanceCount, uint32 startVertex, uint32 startInstance)
{
    FlushResourceBarriers();

    for (int i = 0; i < 2; ++i)
        m_pDynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);

    m_d3d12CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//
// Modified by: Jonathan Åleskog

#include "PreCompiled.h"
#include "DXCommandContext.h"
#include "DX12/Final/DXColorBuffer.h"
//#include "DXDepthBuffer.h"
//#include "GraphicsCore.h"
#include "DX12/Final/DXDescriptorHeap.h"
#include "DX12/Final/DXDynamicDescriptorHeap.h"
//#include "EngineProfiling.h"
#include "DX12/Final/DXUploadBuffer.h"
#include "DX12/Final/DXReadbackBuffer.h"

//#pragma warning(push)
//#pragma warning(disable:4100) // unreferenced formal parameters in PIXCopyEventArguments() (WinPixEventRuntime.1.0.200127001)
//#include <pix3.h>
//#pragma warning(pop)

#include "DX12/Final/DXCore.h"

using namespace RS::DX12;

void DXContextManager::DestroyAllContexts(void)
{
    for (uint32_t i = 0; i < 4; ++i)
        sm_ContextPool[i].clear();
}

DXCommandContext* DXContextManager::AllocateContext(D3D12_COMMAND_LIST_TYPE Type)
{
    std::lock_guard<std::mutex> LockGuard(sm_ContextAllocationMutex);

    auto& AvailableContexts = sm_AvailableContexts[Type];

    DXCommandContext* ret = nullptr;
    if (AvailableContexts.empty())
    {
        ret = new DXCommandContext(Type);
        sm_ContextPool[Type].emplace_back(ret);
        ret->Initialize();
    }
    else
    {
        ret = AvailableContexts.front();
        AvailableContexts.pop();
        ret->Reset();
    }
    RS_ASSERT(ret != nullptr);

    RS_ASSERT(ret->m_Type == Type);

    return ret;
}

void DXContextManager::FreeContext(DXCommandContext* UsedContext)
{
    RS_ASSERT(UsedContext != nullptr);
    std::lock_guard<std::mutex> LockGuard(sm_ContextAllocationMutex);
    sm_AvailableContexts[UsedContext->m_Type].push(UsedContext);
}

void DXCommandContext::DestroyAllContexts(void)
{
    DXLinearAllocator::DestroyAll();
    DXDynamicDescriptorHeap::DestroyAll();
    DXCore::GetContextManager()->DestroyAllContexts();
}

DXCommandContext& DXCommandContext::Begin(const std::wstring ID)
{
    DXCommandContext* NewContext = DXCore::GetContextManager()->AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
    NewContext->SetID(ID);
    //if (ID.length() > 0)
    //    DXEngineProfiling::BeginBlock(ID, NewContext);
    return *NewContext;
}

DXComputeContext& DXComputeContext::Begin(const std::wstring& ID, bool Async)
{
    DXComputeContext& NewContext = DXCore::GetContextManager()->AllocateContext(
        Async ? D3D12_COMMAND_LIST_TYPE_COMPUTE : D3D12_COMMAND_LIST_TYPE_DIRECT)->GetComputeContext();
    NewContext.SetID(ID);
    //if (ID.length() > 0)
    //    DXEngineProfiling::BeginBlock(ID, &NewContext);
    return NewContext;
}

uint64_t DXCommandContext::Flush(bool WaitForCompletion)
{
    FlushResourceBarriers();

    RS_ASSERT(m_CurrentAllocator != nullptr);

    uint64_t FenceValue = DXCore::GetCommandListManager()->GetQueue(m_Type).ExecuteCommandList(m_CommandList);

    if (WaitForCompletion)
        DXCore::GetCommandListManager()->WaitForFence(FenceValue);

    //
    // Reset the command list and restore previous state
    //

    m_CommandList->Reset(m_CurrentAllocator, nullptr);

    if (m_CurGraphicsRootSignature)
    {
        m_CommandList->SetGraphicsRootSignature(m_CurGraphicsRootSignature);
    }
    if (m_CurComputeRootSignature)
    {
        m_CommandList->SetComputeRootSignature(m_CurComputeRootSignature);
    }
    if (m_CurPipelineState)
    {
        m_CommandList->SetPipelineState(m_CurPipelineState);
    }

    BindDescriptorHeaps();

    return FenceValue;
}

uint64_t DXCommandContext::Finish(bool WaitForCompletion)
{
    RS_ASSERT(m_Type == D3D12_COMMAND_LIST_TYPE_DIRECT || m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE);

    FlushResourceBarriers();

    //if (m_ID.length() > 0)
    //    DXEngineProfiling::EndBlock(this);

    RS_ASSERT(m_CurrentAllocator != nullptr);

    DXCommandQueue& Queue = DXCore::GetCommandListManager()->GetQueue(m_Type);

    uint64_t FenceValue = Queue.ExecuteCommandList(m_CommandList);
    Queue.DiscardAllocator(FenceValue, m_CurrentAllocator);
    m_CurrentAllocator = nullptr;

    m_CpuLinearAllocator.CleanupUsedPages(FenceValue);
    m_GpuLinearAllocator.CleanupUsedPages(FenceValue);
    m_pDynamicViewDescriptorHeap->CleanupUsedHeaps(FenceValue);
    m_pDynamicSamplerDescriptorHeap->CleanupUsedHeaps(FenceValue);

    if (WaitForCompletion)
        DXCore::GetCommandListManager()->WaitForFence(FenceValue);

    DXCore::GetContextManager()->FreeContext(this);

    return FenceValue;
}

DXCommandContext::DXCommandContext(D3D12_COMMAND_LIST_TYPE Type) :
    m_Type(Type),
    m_pDynamicViewDescriptorHeap(new DXDynamicDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
    m_pDynamicSamplerDescriptorHeap(new DXDynamicDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)),
    m_CpuLinearAllocator(DXLinearAllocatorType::CPUWritable),
    m_GpuLinearAllocator(DXLinearAllocatorType::GPUExclusive)
{
    m_OwningManager = nullptr;
    m_CommandList = nullptr;
    m_CurrentAllocator = nullptr;
    ZeroMemory(m_CurrentDescriptorHeaps, sizeof(m_CurrentDescriptorHeaps));

    m_CurGraphicsRootSignature = nullptr;
    m_CurComputeRootSignature = nullptr;
    m_CurPipelineState = nullptr;
    m_NumBarriersToFlush = 0;
}

DXCommandContext::~DXCommandContext(void)
{
    if (m_CommandList != nullptr)
        m_CommandList->Release();

    delete m_pDynamicViewDescriptorHeap;
    m_pDynamicViewDescriptorHeap = nullptr;
    delete m_pDynamicSamplerDescriptorHeap;
    m_pDynamicSamplerDescriptorHeap = nullptr;
}

void DXCommandContext::Initialize(void)
{
    DXCore::GetCommandListManager()->CreateNewCommandList(m_Type, &m_CommandList, &m_CurrentAllocator);
}

void DXCommandContext::Reset(void)
{
    // We only call Reset() on previously freed contexts.  The command list persists, but we must
    // request a new allocator.
    RS_ASSERT(m_CommandList != nullptr && m_CurrentAllocator == nullptr);
    m_CurrentAllocator = DXCore::GetCommandListManager()->GetQueue(m_Type).RequestAllocator();
    m_CommandList->Reset(m_CurrentAllocator, nullptr);

    m_CurGraphicsRootSignature = nullptr;
    m_CurComputeRootSignature = nullptr;
    m_CurPipelineState = nullptr;
    m_NumBarriersToFlush = 0;

    BindDescriptorHeaps();
}

void DXCommandContext::BindDescriptorHeaps(void)
{
    UINT NonNullHeaps = 0;
    ID3D12DescriptorHeap* HeapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    for (UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        ID3D12DescriptorHeap* HeapIter = m_CurrentDescriptorHeaps[i];
        if (HeapIter != nullptr)
            HeapsToBind[NonNullHeaps++] = HeapIter;
    }

    if (NonNullHeaps > 0)
        m_CommandList->SetDescriptorHeaps(NonNullHeaps, HeapsToBind);
}

void DXGraphicsContext::SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{
    m_CommandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, &DSV);
}

void DXGraphicsContext::SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[])
{
    m_CommandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, nullptr);
}

void DXGraphicsContext::BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
{
    m_CommandList->BeginQuery(QueryHeap, Type, HeapIndex);
}

void DXGraphicsContext::EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
{
    m_CommandList->EndQuery(QueryHeap, Type, HeapIndex);
}

void DXGraphicsContext::ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset)
{
    m_CommandList->ResolveQueryData(QueryHeap, Type, StartIndex, NumQueries, DestinationBuffer, DestinationBufferOffset);
}

void RS::DX12::DXGraphicsContext::SetRootSignature(const DXRootSignature& RootSig)
{
    if (RootSig.GetSignature() == m_CurGraphicsRootSignature)
        return;

    m_CommandList->SetGraphicsRootSignature(m_CurGraphicsRootSignature = RootSig.GetSignature());

    m_pDynamicViewDescriptorHeap->ParseGraphicsRootSignature(RootSig);
    m_pDynamicSamplerDescriptorHeap->ParseGraphicsRootSignature(RootSig);
}

void DXGraphicsContext::ClearUAV(DXGPUBuffer& Target)
{
    FlushResourceBarriers();

    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_pDynamicViewDescriptorHeap->UploadDirect(Target.GetUAV());
    const UINT ClearColor[4] = {};
    m_CommandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
}

void DXComputeContext::ClearUAV(DXGPUBuffer& Target)
{
    FlushResourceBarriers();

    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_pDynamicViewDescriptorHeap->UploadDirect(Target.GetUAV());
    const UINT ClearColor[4] = {};
    m_CommandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
}

void DXGraphicsContext::ClearUAV(DXColorBuffer& Target)
{
    FlushResourceBarriers();

    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_pDynamicViewDescriptorHeap->UploadDirect(Target.GetUAV());
    CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

    //TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
    const glm::vec4 color = Color::ToVec4(Target.GetClearColor());
    const float* ClearColor = (const float*)&color.x;
    m_CommandList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 1, &ClearRect);
}

void DXComputeContext::ClearUAV(DXColorBuffer& Target)
{
    FlushResourceBarriers();

    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_pDynamicViewDescriptorHeap->UploadDirect(Target.GetUAV());
    CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

    //TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
    const glm::vec4 color = Color::ToVec4(Target.GetClearColor());
    const float* ClearColor = (const float*)&color.x;
    m_CommandList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 1, &ClearRect);
}

void RS::DX12::DXComputeContext::SetRootSignature(const DXRootSignature& RootSig)
{
    if (RootSig.GetSignature() == m_CurComputeRootSignature)
        return;

    m_CommandList->SetComputeRootSignature(m_CurComputeRootSignature = RootSig.GetSignature());

    m_pDynamicViewDescriptorHeap->ParseComputeRootSignature(RootSig);
    m_pDynamicSamplerDescriptorHeap->ParseComputeRootSignature(RootSig);
}

void RS::DX12::DXComputeContext::Dispatch(size_t GroupCountX, size_t GroupCountY, size_t GroupCountZ)
{
    FlushResourceBarriers();
    m_pDynamicViewDescriptorHeap->CommitComputeRootDescriptorTables(m_CommandList);
    m_pDynamicSamplerDescriptorHeap->CommitComputeRootDescriptorTables(m_CommandList);
    m_CommandList->Dispatch((UINT)GroupCountX, (UINT)GroupCountY, (UINT)GroupCountZ);
}

void RS::DX12::DXGraphicsContext::SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
    m_pDynamicViewDescriptorHeap->SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
}

void RS::DX12::DXComputeContext::SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
    m_pDynamicViewDescriptorHeap->SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
}

void RS::DX12::DXGraphicsContext::SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
    m_pDynamicSamplerDescriptorHeap->SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
}

void RS::DX12::DXComputeContext::SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
    m_pDynamicSamplerDescriptorHeap->SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
}

void RS::DX12::DXGraphicsContext::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
    UINT StartVertexLocation, UINT StartInstanceLocation)
{
    FlushResourceBarriers();
    m_pDynamicViewDescriptorHeap->CommitGraphicsRootDescriptorTables(m_CommandList);
    m_pDynamicSamplerDescriptorHeap->CommitGraphicsRootDescriptorTables(m_CommandList);
    m_CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void RS::DX12::DXGraphicsContext::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
    INT BaseVertexLocation, UINT StartInstanceLocation)
{
    FlushResourceBarriers();
    m_pDynamicViewDescriptorHeap->CommitGraphicsRootDescriptorTables(m_CommandList);
    m_pDynamicSamplerDescriptorHeap->CommitGraphicsRootDescriptorTables(m_CommandList);
    m_CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void RS::DX12::DXGraphicsContext::ExecuteIndirect(DXCommandSignature& CommandSig,
    DXGPUBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset,
    uint32_t MaxCommands, DXGPUBuffer* CommandCounterBuffer, uint64_t CounterOffset)
{
    FlushResourceBarriers();
    m_pDynamicViewDescriptorHeap->CommitGraphicsRootDescriptorTables(m_CommandList);
    m_pDynamicSamplerDescriptorHeap->CommitGraphicsRootDescriptorTables(m_CommandList);
    m_CommandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands,
        ArgumentBuffer.GetResource(), ArgumentStartOffset,
        CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(), CounterOffset);
}

void RS::DX12::DXComputeContext::ExecuteIndirect(DXCommandSignature& CommandSig,
    DXGPUBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset,
    uint32_t MaxCommands, DXGPUBuffer* CommandCounterBuffer, uint64_t CounterOffset)
{
    FlushResourceBarriers();
    m_pDynamicViewDescriptorHeap->CommitComputeRootDescriptorTables(m_CommandList);
    m_pDynamicSamplerDescriptorHeap->CommitComputeRootDescriptorTables(m_CommandList);
    m_CommandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands,
        ArgumentBuffer.GetResource(), ArgumentStartOffset,
        CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(), CounterOffset);
}

void DXGraphicsContext::ClearColor(DXColorBuffer& Target, D3D12_RECT* Rect)
{
    FlushResourceBarriers();

    const glm::vec4 color = Color::ToVec4(Target.GetClearColor());
    const float* ClearColor = (const float*)&color.x;
    m_CommandList->ClearRenderTargetView(Target.GetRTV(), ClearColor, (Rect == nullptr) ? 0 : 1, Rect);
}

void DXGraphicsContext::ClearColor(DXColorBuffer& Target, float Colour[4], D3D12_RECT* Rect)
{
    FlushResourceBarriers();
    m_CommandList->ClearRenderTargetView(Target.GetRTV(), Colour, (Rect == nullptr) ? 0 : 1, Rect);
}

void DXGraphicsContext::ClearDepth(DXDepthBuffer& Target)
{
    FlushResourceBarriers();
    m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
}

void DXGraphicsContext::ClearStencil(DXDepthBuffer& Target)
{
    FlushResourceBarriers();
    m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_STENCIL, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
}

void DXGraphicsContext::ClearDepthAndStencil(DXDepthBuffer& Target)
{
    FlushResourceBarriers();
    m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
}

void DXGraphicsContext::SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
    RS_ASSERT(rect.left < rect.right && rect.top < rect.bottom);
    m_CommandList->RSSetViewports(1, &vp);
    m_CommandList->RSSetScissorRects(1, &rect);
}

void DXGraphicsContext::SetViewport(const D3D12_VIEWPORT& vp)
{
    m_CommandList->RSSetViewports(1, &vp);
}

void DXGraphicsContext::SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth, FLOAT maxDepth)
{
    D3D12_VIEWPORT vp;
    vp.Width = w;
    vp.Height = h;
    vp.MinDepth = minDepth;
    vp.MaxDepth = maxDepth;
    vp.TopLeftX = x;
    vp.TopLeftY = y;
    m_CommandList->RSSetViewports(1, &vp);
}

void DXGraphicsContext::SetScissor(const D3D12_RECT& rect)
{
    RS_ASSERT(rect.left < rect.right && rect.top < rect.bottom);
    m_CommandList->RSSetScissorRects(1, &rect);
}

void DXCommandContext::TransitionResource(DXGPUResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
{
    D3D12_RESOURCE_STATES OldState = Resource.m_UsageState;

    if (m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        RS_ASSERT((OldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == OldState);
        RS_ASSERT((NewState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == NewState);
    }

    if (OldState != NewState)
    {
        RS_ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource = Resource.GetResource();
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = OldState;
        BarrierDesc.Transition.StateAfter = NewState;

        // Check to see if we already started the transition
        if (NewState == Resource.m_TransitioningState)
        {
            BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
            Resource.m_TransitioningState = (D3D12_RESOURCE_STATES)-1;
        }
        else
            BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

        Resource.m_UsageState = NewState;
    }
    else if (NewState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        InsertUAVBarrier(Resource, FlushImmediate);

    if (FlushImmediate || m_NumBarriersToFlush == 16)
        FlushResourceBarriers();
}

void DXCommandContext::BeginResourceTransition(DXGPUResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
{
    // If it's already transitioning, finish that transition
    if (Resource.m_TransitioningState != (D3D12_RESOURCE_STATES)-1)
        TransitionResource(Resource, Resource.m_TransitioningState);

    D3D12_RESOURCE_STATES OldState = Resource.m_UsageState;

    if (OldState != NewState)
    {
        RS_ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource = Resource.GetResource();
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = OldState;
        BarrierDesc.Transition.StateAfter = NewState;

        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

        Resource.m_TransitioningState = NewState;
    }

    if (FlushImmediate || m_NumBarriersToFlush == 16)
        FlushResourceBarriers();
}

void DXCommandContext::InsertUAVBarrier(DXGPUResource& Resource, bool FlushImmediate)
{
    RS_ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

    BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    BarrierDesc.UAV.pResource = Resource.GetResource();

    if (FlushImmediate)
        FlushResourceBarriers();
}

void DXCommandContext::InsertAliasBarrier(DXGPUResource& Before, DXGPUResource& After, bool FlushImmediate)
{
    RS_ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

    BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    BarrierDesc.Aliasing.pResourceBefore = Before.GetResource();
    BarrierDesc.Aliasing.pResourceAfter = After.GetResource();

    if (FlushImmediate)
        FlushResourceBarriers();
}

void DXCommandContext::WriteBuffer(DXGPUResource& Dest, size_t DestOffset, const void* BufferData, size_t NumBytes)
{
    RS_ASSERT(BufferData != nullptr && Utils::IsAligned(BufferData, 16));
    DXDynAlloc TempSpace = m_CpuLinearAllocator.Allocate(NumBytes, 512);
    Utils::SIMDMemCopy(TempSpace.DataPtr, BufferData, Utils::DivideByMultiple(NumBytes, 16));
    CopyBufferRegion(Dest, DestOffset, TempSpace.Buffer, TempSpace.Offset, NumBytes);
}

void DXCommandContext::FillBuffer(DXGPUResource& Dest, size_t DestOffset, DXDWParam Value, size_t NumBytes)
{
    DXDynAlloc TempSpace = m_CpuLinearAllocator.Allocate(NumBytes, 512);
    __m128 VectorValue = _mm_set1_ps(Value.Float);
    Utils::SIMDMemFill(TempSpace.DataPtr, VectorValue, Utils::DivideByMultiple(NumBytes, 16));
    CopyBufferRegion(Dest, DestOffset, TempSpace.Buffer, TempSpace.Offset, NumBytes);
}

void DXCommandContext::InitializeTexture(DXGPUResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[])
{
    UINT64 uploadBufferSize = GetRequiredIntermediateSize(Dest.GetResource(), 0, NumSubresources);

    DXCommandContext& InitContext = DXCommandContext::Begin();

    // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
    DXDynAlloc mem = InitContext.ReserveUploadMemory(uploadBufferSize);
    UpdateSubresources(InitContext.m_CommandList, Dest.GetResource(), mem.Buffer.GetResource(), 0, 0, NumSubresources, SubData);
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);

    // Execute the command list and wait for it to finish so we can release the upload buffer
    InitContext.Finish(true);
}

void DXCommandContext::CopySubresource(DXGPUResource& Dest, UINT DestSubIndex, DXGPUResource& Src, UINT SrcSubIndex)
{
    FlushResourceBarriers();

    D3D12_TEXTURE_COPY_LOCATION DestLocation =
    {
        Dest.GetResource(),
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        DestSubIndex
    };

    D3D12_TEXTURE_COPY_LOCATION SrcLocation =
    {
        Src.GetResource(),
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        SrcSubIndex
    };

    m_CommandList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SrcLocation, nullptr);
}

void DXCommandContext::InitializeTextureArraySlice(DXGPUResource& Dest, UINT SliceIndex, DXGPUResource& Src)
{
    DXCommandContext& Context = DXCommandContext::Begin();

    Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    Context.FlushResourceBarriers();

    const D3D12_RESOURCE_DESC& DestDesc = Dest.GetResource()->GetDesc();
    const D3D12_RESOURCE_DESC& SrcDesc = Src.GetResource()->GetDesc();

    RS_ASSERT(SliceIndex < DestDesc.DepthOrArraySize &&
        SrcDesc.DepthOrArraySize == 1 &&
        DestDesc.Width == SrcDesc.Width &&
        DestDesc.Height == SrcDesc.Height &&
        DestDesc.MipLevels <= SrcDesc.MipLevels
    );

    UINT SubResourceIndex = SliceIndex * DestDesc.MipLevels;

    for (UINT i = 0; i < DestDesc.MipLevels; ++i)
    {
        D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
        {
            Dest.GetResource(),
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            SubResourceIndex + i
        };

        D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
        {
            Src.GetResource(),
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            i
        };

        Context.m_CommandList->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);
    }

    Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);
    Context.Finish(true);
}

uint32_t DXCommandContext::ReadbackTexture(DXReadbackBuffer& DstBuffer, DXPixelBuffer& SrcBuffer)
{
    uint64_t CopySize = 0;

    // The footprint may depend on the device of the resource, but we assume there is only one device.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;
    D3D12_RESOURCE_DESC srcBufferDesc = SrcBuffer.GetResource()->GetDesc();
    DXCore::GetDevice()->GetCopyableFootprints(&srcBufferDesc, 0, 1, 0,
        &PlacedFootprint, nullptr, nullptr, &CopySize);

    DstBuffer.Create(L"Readback", (uint32_t)CopySize, 1);

    TransitionResource(SrcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, true);

    CD3DX12_TEXTURE_COPY_LOCATION dstCopyLocation(DstBuffer.GetResource(), PlacedFootprint);
    CD3DX12_TEXTURE_COPY_LOCATION srcCopyLocation(SrcBuffer.GetResource(), 0);
    m_CommandList->CopyTextureRegion(
        &dstCopyLocation, 0, 0, 0,
        &srcCopyLocation, nullptr);

    return PlacedFootprint.Footprint.RowPitch;
}

void DXCommandContext::InitializeBuffer(DXGPUBuffer& Dest, const void* BufferData, size_t NumBytes, size_t DestOffset)
{
    DXCommandContext& InitContext = DXCommandContext::Begin();

    DXDynAlloc mem = InitContext.ReserveUploadMemory(NumBytes);
    Utils::SIMDMemCopy(mem.DataPtr, BufferData, Utils::DivideByMultiple(NumBytes, 16));

    // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
    InitContext.m_CommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, mem.Buffer.GetResource(), 0, NumBytes);
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

    // Execute the command list and wait for it to finish so we can release the upload buffer
    InitContext.Finish(true);
}

void DXCommandContext::InitializeBuffer(DXGPUBuffer& Dest, const DXUploadBuffer& Src, size_t SrcOffset, size_t NumBytes, size_t DestOffset)
{
    DXCommandContext& InitContext = DXCommandContext::Begin();

    size_t MaxBytes = std::min<size_t>(Dest.GetBufferSize() - DestOffset, Src.GetBufferSize() - SrcOffset);
    NumBytes = std::min<size_t>(MaxBytes, NumBytes);

    // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
    InitContext.m_CommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, (ID3D12Resource*)Src.GetResource(), SrcOffset, NumBytes);
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

    // Execute the command list and wait for it to finish so we can release the upload buffer
    InitContext.Finish(true);
}

void DXCommandContext::PIXBeginEvent(const wchar_t* label)
{
#ifdef RS_CONFIG_DEVELOPMENT
    //::PIXBeginEvent(m_CommandList, 0, label);
#else
    RS_UNUSED(lable);
#endif
}

void DXCommandContext::PIXEndEvent(void)
{
#ifdef RS_CONFIG_DEVELOPMENT
    //::PIXEndEvent(m_CommandList);
#endif
}

void DXCommandContext::PIXSetMarker(const wchar_t* label)
{
#ifdef RS_CONFIG_DEVELOPMENT
    //::PIXSetMarker(m_CommandList, 0, label);
#else
    RS_UNUSED(label);
#endif
}

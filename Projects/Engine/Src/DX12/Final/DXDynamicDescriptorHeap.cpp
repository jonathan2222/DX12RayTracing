#include "PreCompiled.h"
#include "DXDynamicDescriptorHeap.h"

#include "DX12/Final/DXCommandContext.h"
#include "DX12/Final/DXCore.h"
#include "DX12/Final/DXCommandListManager.h"
#include "DX12/Final/DXRootSignature.h"

std::mutex RS::DX12::DXDynamicDescriptorHeap::sm_Mutex;
std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> RS::DX12::DXDynamicDescriptorHeap::sm_DescriptorHeapPool[2];
std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>> RS::DX12::DXDynamicDescriptorHeap::sm_RetiredDescriptorHeaps[2];
std::queue<ID3D12DescriptorHeap*> RS::DX12::DXDynamicDescriptorHeap::sm_AvailableDescriptorHeaps[2];

RS::DX12::DXDynamicDescriptorHeap::DXDynamicDescriptorHeap(DXCommandContext* pOwningContext, D3D12_DESCRIPTOR_HEAP_TYPE HeapType)
    : m_pOwningContext(pOwningContext), m_DescriptorType(HeapType)
{
    m_CurrentHeapPtr = nullptr;
    m_CurrentOffset = 0;
    m_DescriptorSize = DXCore::GetDevice()->GetDescriptorHandleIncrementSize(HeapType);
}

RS::DX12::DXDynamicDescriptorHeap::~DXDynamicDescriptorHeap()
{
}

void RS::DX12::DXDynamicDescriptorHeap::CleanupUsedHeaps(uint64_t fenceValue)
{
    RetireCurrentHeap();
    RetireUsedHeaps(fenceValue);
    m_GraphicsHandleCache.ClearCache();
    m_ComputeHandleCache.ClearCache();
}

D3D12_GPU_DESCRIPTOR_HANDLE RS::DX12::DXDynamicDescriptorHeap::UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
    if (!HasSpace(1))
    {
        RetireCurrentHeap();
        UnbindAllValid();
    }

    m_pOwningContext->SetDescriptorHeap(m_DescriptorType, GetHeapPointer());

    DXDescriptorHandle DestHandle = m_FirstDescriptor + m_CurrentOffset * m_DescriptorSize;
    m_CurrentOffset += 1;

    DXCore::GetDevice()->CopyDescriptorsSimple(1, DestHandle, Handle, m_DescriptorType);

    return DestHandle;
}

ID3D12DescriptorHeap* RS::DX12::DXDynamicDescriptorHeap::RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType)
{
    std::lock_guard<std::mutex> LockGuard(sm_Mutex);

    uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

    while (!sm_RetiredDescriptorHeaps[idx].empty() && DXCore::GetCommandListManager()->IsFenceComplete(sm_RetiredDescriptorHeaps[idx].front().first))
    {
        sm_AvailableDescriptorHeaps[idx].push(sm_RetiredDescriptorHeaps[idx].front().second);
        sm_RetiredDescriptorHeaps[idx].pop();
    }

    if (!sm_AvailableDescriptorHeaps[idx].empty())
    {
        ID3D12DescriptorHeap* HeapPtr = sm_AvailableDescriptorHeaps[idx].front();
        sm_AvailableDescriptorHeaps[idx].pop();
        return HeapPtr;
    }
    else
    {
        D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
        HeapDesc.Type = HeapType;
        HeapDesc.NumDescriptors = kNumDescriptorsPerHeap;
        HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        HeapDesc.NodeMask = 1;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> HeapPtr;
        DXCall(DXCore::GetDevice()->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&HeapPtr)));
        sm_DescriptorHeapPool[idx].emplace_back(HeapPtr);
        return HeapPtr.Get();
    }
}

void RS::DX12::DXDynamicDescriptorHeap::DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint64_t FenceValueForReset, const std::vector<ID3D12DescriptorHeap*>& UsedHeaps)
{
    uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;
    std::lock_guard<std::mutex> LockGuard(sm_Mutex);
    for (auto iter = UsedHeaps.begin(); iter != UsedHeaps.end(); ++iter)
        sm_RetiredDescriptorHeaps[idx].push(std::make_pair(FenceValueForReset, *iter));
}

void RS::DX12::DXDynamicDescriptorHeap::RetireCurrentHeap(void)
{
    // Don't retire unused heaps.
    if (m_CurrentOffset == 0)
    {
        RS_ASSERT(m_CurrentHeapPtr == nullptr);
        return;
    }

    RS_ASSERT(m_CurrentHeapPtr != nullptr);
    m_RetiredHeaps.push_back(m_CurrentHeapPtr);
    m_CurrentHeapPtr = nullptr;
    m_CurrentOffset = 0;
}

void RS::DX12::DXDynamicDescriptorHeap::RetireUsedHeaps(uint64_t fenceValue)
{
    DiscardDescriptorHeaps(m_DescriptorType, fenceValue, m_RetiredHeaps);
    m_RetiredHeaps.clear();
}

ID3D12DescriptorHeap* RS::DX12::DXDynamicDescriptorHeap::GetHeapPointer()
{
    if (m_CurrentHeapPtr == nullptr)
    {
        RS_ASSERT(m_CurrentOffset == 0);
        m_CurrentHeapPtr = RequestDescriptorHeap(m_DescriptorType);
        m_FirstDescriptor = DXDescriptorHandle(
            m_CurrentHeapPtr->GetCPUDescriptorHandleForHeapStart(),
            m_CurrentHeapPtr->GetGPUDescriptorHandleForHeapStart());
    }

    return m_CurrentHeapPtr;
}

void RS::DX12::DXDynamicDescriptorHeap::CopyAndBindStagedTables(DescriptorHandleCache& HandleCache, ID3D12GraphicsCommandList* CmdList, void(__stdcall ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
{
    uint32_t NeededSize = HandleCache.ComputeStagedSize();
    if (!HasSpace(NeededSize))
    {
        RetireCurrentHeap();
        UnbindAllValid();
        NeededSize = HandleCache.ComputeStagedSize();
    }

    // This can trigger the creation of a new heap
    m_pOwningContext->SetDescriptorHeap(m_DescriptorType, GetHeapPointer());
    HandleCache.CopyAndBindStaleTables(m_DescriptorType, m_DescriptorSize, Allocate(NeededSize), CmdList, SetFunc);
}

void RS::DX12::DXDynamicDescriptorHeap::UnbindAllValid()
{
    m_GraphicsHandleCache.UnbindAllValid();
    m_ComputeHandleCache.UnbindAllValid();
}

uint32_t RS::DX12::DXDynamicDescriptorHeap::DescriptorHandleCache::ComputeStagedSize()
{
    // Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
    uint32_t NeededSpace = 0;
    uint32_t RootIndex;
    uint32_t StaleParams = m_StaleRootParamsBitMap;
    while (_BitScanForward((unsigned long*)&RootIndex, StaleParams))
    {
        StaleParams ^= (1 << RootIndex);

        uint32_t MaxSetHandle;
        RS_ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap),
            "Root entry marked as stale but has no stale descriptors");

        NeededSpace += MaxSetHandle + 1;
    }
    return NeededSpace;
}

void RS::DX12::DXDynamicDescriptorHeap::DescriptorHandleCache::CopyAndBindStaleTables(D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t DescriptorSize, DXDescriptorHandle DestHandleStart, ID3D12GraphicsCommandList* CmdList, void(__stdcall ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
{
    uint32_t StaleParamCount = 0;
    uint32_t TableSize[DescriptorHandleCache::kMaxNumDescriptorTables];
    uint32_t RootIndices[DescriptorHandleCache::kMaxNumDescriptorTables];
    uint32_t NeededSpace = 0;
    uint32_t RootIndex;

    // Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
    uint32_t StaleParams = m_StaleRootParamsBitMap;
    while (_BitScanForward((unsigned long*)&RootIndex, StaleParams))
    {
        RootIndices[StaleParamCount] = RootIndex;
        StaleParams ^= (1 << RootIndex);

        uint32_t MaxSetHandle;
        RS_ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap),
            "Root entry marked as stale but has no stale descriptors");

        NeededSpace += MaxSetHandle + 1;
        TableSize[StaleParamCount] = MaxSetHandle + 1;

        ++StaleParamCount;
    }

    RS_ASSERT(StaleParamCount <= DescriptorHandleCache::kMaxNumDescriptorTables,
        "We're only equipped to handle so many descriptor tables");

    m_StaleRootParamsBitMap = 0;

    static const uint32_t kMaxDescriptorsPerCopy = 16;
    UINT NumDestDescriptorRanges = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[kMaxDescriptorsPerCopy];
    UINT pDestDescriptorRangeSizes[kMaxDescriptorsPerCopy];

    UINT NumSrcDescriptorRanges = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE pSrcDescriptorRangeStarts[kMaxDescriptorsPerCopy];
    UINT pSrcDescriptorRangeSizes[kMaxDescriptorsPerCopy];

    for (uint32_t i = 0; i < StaleParamCount; ++i)
    {
        RootIndex = RootIndices[i];
        (CmdList->*SetFunc)(RootIndex, DestHandleStart);

        DescriptorTableCache& RootDescTable = m_RootDescriptorTable[RootIndex];

        D3D12_CPU_DESCRIPTOR_HANDLE* SrcHandles = RootDescTable.TableStart;
        uint64_t SetHandles = (uint64_t)RootDescTable.AssignedHandlesBitMap;
        D3D12_CPU_DESCRIPTOR_HANDLE CurDest = DestHandleStart;
        DestHandleStart += TableSize[i] * DescriptorSize;

        unsigned long SkipCount;
        while (_BitScanForward64(&SkipCount, SetHandles))
        {
            // Skip over unset descriptor handles
            SetHandles >>= SkipCount;
            SrcHandles += SkipCount;
            CurDest.ptr += SkipCount * DescriptorSize;

            unsigned long DescriptorCount;
            _BitScanForward64(&DescriptorCount, ~SetHandles);
            SetHandles >>= DescriptorCount;

            // If we run out of temp room, copy what we've got so far
            if (NumSrcDescriptorRanges + DescriptorCount > kMaxDescriptorsPerCopy)
            {
                DXCore::GetDevice()->CopyDescriptors(
                    NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
                    NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
                    Type);

                NumSrcDescriptorRanges = 0;
                NumDestDescriptorRanges = 0;
            }

            // Setup destination range
            pDestDescriptorRangeStarts[NumDestDescriptorRanges] = CurDest;
            pDestDescriptorRangeSizes[NumDestDescriptorRanges] = DescriptorCount;
            ++NumDestDescriptorRanges;

            // Setup source ranges (one descriptor each because we don't assume they are contiguous)
            for (uint32_t j = 0; j < DescriptorCount; ++j)
            {
                pSrcDescriptorRangeStarts[NumSrcDescriptorRanges] = SrcHandles[j];
                pSrcDescriptorRangeSizes[NumSrcDescriptorRanges] = 1;
                ++NumSrcDescriptorRanges;
            }

            // Move the destination pointer forward by the number of descriptors we will copy
            SrcHandles += DescriptorCount;
            CurDest.ptr += DescriptorCount * DescriptorSize;
        }
    }

    DXCore::GetDevice()->CopyDescriptors(
        NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
        NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
        Type);
}

void RS::DX12::DXDynamicDescriptorHeap::DescriptorHandleCache::UnbindAllValid()
{
    m_StaleRootParamsBitMap = 0;

    unsigned long TableParams = m_RootDescriptorTablesBitMap;
    unsigned long RootIndex;
    while (_BitScanForward(&RootIndex, TableParams))
    {
        TableParams ^= (1 << RootIndex);
        if (m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap != 0)
            m_StaleRootParamsBitMap |= (1 << RootIndex);
    }
}

void RS::DX12::DXDynamicDescriptorHeap::DescriptorHandleCache::StageDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
    RS_ASSERT(((1 << RootIndex) & m_RootDescriptorTablesBitMap) != 0, "Root parameter is not a CBV_SRV_UAV descriptor table");
    RS_ASSERT(Offset + NumHandles <= m_RootDescriptorTable[RootIndex].TableSize);

    DescriptorTableCache& TableCache = m_RootDescriptorTable[RootIndex];
    D3D12_CPU_DESCRIPTOR_HANDLE* CopyDest = TableCache.TableStart + Offset;
    for (UINT i = 0; i < NumHandles; ++i)
        CopyDest[i] = Handles[i];
    TableCache.AssignedHandlesBitMap |= ((1 << NumHandles) - 1) << Offset;
    m_StaleRootParamsBitMap |= (1 << RootIndex);
}

void RS::DX12::DXDynamicDescriptorHeap::DescriptorHandleCache::ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE Type, const DXRootSignature& RootSig)
{
    UINT CurrentOffset = 0;

    RS_ASSERT(RootSig.m_NumParameters <= 16, "Maybe we need to support something greater");

    m_StaleRootParamsBitMap = 0;
    m_RootDescriptorTablesBitMap = (Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
        RootSig.m_SamplerTableBitMap : RootSig.m_DescriptorTableBitMap);

    unsigned long TableParams = m_RootDescriptorTablesBitMap;
    unsigned long RootIndex;
    while (_BitScanForward(&RootIndex, TableParams))
    {
        TableParams ^= (1 << RootIndex);

        UINT TableSize = RootSig.m_DescriptorTableSize[RootIndex];
        RS_ASSERT(TableSize > 0);

        DescriptorTableCache& RootDescriptorTable = m_RootDescriptorTable[RootIndex];
        RootDescriptorTable.AssignedHandlesBitMap = 0;
        RootDescriptorTable.TableStart = m_HandleCache + CurrentOffset;
        RootDescriptorTable.TableSize = TableSize;

        CurrentOffset += TableSize;
    }

    m_MaxCachedDescriptors = CurrentOffset;

    RS_ASSERT(m_MaxCachedDescriptors <= kMaxNumDescriptors, "Exceeded user-supplied maximum cache size");
}

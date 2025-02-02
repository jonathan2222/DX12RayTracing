#include "PreCompiled.h"
#include "DXDescriptorHeap.h"

#include "DX12/Final/DXCommandListManager.h"
#include "DX12/Final/DXCore.h"

//
// DescriptorAllocator implementation
//
std::mutex RS::DX12::DXDescriptorAllocator::sm_AllocationMutex;
std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> RS::DX12::DXDescriptorAllocator::sm_DescriptorHeapPool;

D3D12_CPU_DESCRIPTOR_HANDLE RS::DX12::DXDescriptorAllocator::Allocate(uint32 count)
{
    if (m_CurrentHeap == nullptr || m_RemainingFreeHandles < count)
    {
        m_CurrentHeap = RequestNewHeap(m_Type);
        m_CurrentHandle = m_CurrentHeap->GetCPUDescriptorHandleForHeapStart();
        m_RemainingFreeHandles = sm_NumDescriptorsPerHeap;

        if (m_DescriptorSize == 0)
            m_DescriptorSize = DXCore::GetDevice()->GetDescriptorHandleIncrementSize(m_Type);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE ret = m_CurrentHandle;
    m_CurrentHandle.ptr += count * m_DescriptorSize;
    m_RemainingFreeHandles -= count;
    return ret;
}

void RS::DX12::DXDescriptorAllocator::DestroyAll()
{
    sm_DescriptorHeapPool.clear();
}

ID3D12DescriptorHeap* RS::DX12::DXDescriptorAllocator::RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
    std::lock_guard<std::mutex> LockGuard(sm_AllocationMutex);

    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    Desc.Type = Type;
    Desc.NumDescriptors = sm_NumDescriptorsPerHeap;
    Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    Desc.NodeMask = 1;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pHeap;
    DXCall(DXCore::GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pHeap)));
    sm_DescriptorHeapPool.emplace_back(pHeap);
    return pHeap.Get();
}

void RS::DX12::DXDescriptorHeap::Create(const std::wstring& DebugHeapName, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxCount)
{
    m_HeapDesc.Type = Type;
    m_HeapDesc.NumDescriptors = MaxCount;
    m_HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    m_HeapDesc.NodeMask = 1;

    DXCall(DXCore::GetDevice()->CreateDescriptorHeap(&m_HeapDesc, IID_PPV_ARGS(m_Heap.ReleaseAndGetAddressOf())));

#ifdef RS_CONFIG_DEBUG
    m_Heap->SetName(DebugHeapName.c_str());
#else
    (void)Name;
#endif

    m_DescriptorSize = DXCore::GetDevice()->GetDescriptorHandleIncrementSize(m_HeapDesc.Type);
    m_NumFreeDescriptors = m_HeapDesc.NumDescriptors;
    m_FirstHandle = DXDescriptorHandle(
        m_Heap->GetCPUDescriptorHandleForHeapStart(),
        m_Heap->GetGPUDescriptorHandleForHeapStart());
    m_NextFreeHandle = m_FirstHandle;
}

RS::DX12::DXDescriptorHandle RS::DX12::DXDescriptorHeap::Alloc(uint32 count)
{
    RS_ASSERT(HasAvailableSpace(count), "Descriptor Heap out of space.  Increase heap size.");
    DXDescriptorHandle ret = m_NextFreeHandle;
    m_NextFreeHandle += count * m_DescriptorSize;
    m_NumFreeDescriptors -= count;
    return ret;
}

bool RS::DX12::DXDescriptorHeap::ValidateHandle(const DXDescriptorHandle& handle) const
{
    if (handle.GetCpuPtr() < m_FirstHandle.GetCpuPtr() ||
        handle.GetCpuPtr() >= m_FirstHandle.GetCpuPtr() + m_HeapDesc.NumDescriptors * m_DescriptorSize)
        return false;

    if (handle.GetGpuPtr() - m_FirstHandle.GetGpuPtr() !=
        handle.GetCpuPtr() - m_FirstHandle.GetCpuPtr())
        return false;

    return true;
}

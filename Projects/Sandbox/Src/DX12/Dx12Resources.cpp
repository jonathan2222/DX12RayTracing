#include "PreCompiled.h"
#include "Dx12Resources.h"

#include <d3d12.h>
#include "DX12Defines.h"
#include "Dx12Core2.h"

void RS::DX12::Dx12DescriptorHeap::Init(uint32 capacity, bool isShaderVisible, const std::string name)
{
	std::lock_guard lock{ m_Mutex };
	RS_ASSERT_NO_MSG(capacity && capacity < D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2);
	RS_ASSERT_NO_MSG(!(m_Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER && capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE));
	if ((m_Type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV ||
		m_Type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) && isShaderVisible)
	{
		isShaderVisible = false;
		LOG_WARNING("Heap type of {} cannot be shader visible! Setting isShaderVisible to false.", GetDescriptorHeapTypeAsString(m_Type));
	}

	// Release the previous heap if it existed.
	Release();

	ID3D12Device* const pDevice = Dx12Core2::Get()->GetD3D12Device();
	RS_ASSERT_NO_MSG(pDevice);

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Flags = isShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NumDescriptors = capacity;
	desc.Type = m_Type;
	desc.NodeMask = 0;
	DXCall(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap)));
	DX12_SET_DEBUG_NAME_REF(m_Heap, m_DebugName, name.c_str());

	m_FreeHandles = std::move(std::make_unique<uint32[]>(capacity));
	m_Capacity = capacity;
	m_DescriptorCount = 0;

	for (uint32 i = 0; i < capacity; ++i)
		m_FreeHandles[i] = i;

#ifdef RS_CONFIG_DEBUG
	for (uint32 i = 0; i < FRAME_BUFFER_COUNT; ++i)
		RS_ASSERT(m_DeferredFreeIndices[i].empty() == true, "There should be no deferred data to free at this point.");
#endif

	m_DescriptorSize = pDevice->GetDescriptorHandleIncrementSize(m_Type);
	m_CpuStart = m_Heap->GetCPUDescriptorHandleForHeapStart();
	m_GpuStart = isShaderVisible ? m_Heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
}

void RS::DX12::Dx12DescriptorHeap::Release()
{
	RS_ASSERT_NO_MSG(!m_DescriptorCount);
	Dx12Core2::Get()->DeferredRelease(m_Heap);
}

RS::DX12::Dx12DescriptorHandle RS::DX12::Dx12DescriptorHeap::Allocate()
{
	std::lock_guard lock{ m_Mutex };
	RS_ASSERT_NO_MSG(m_Heap);
	RS_ASSERT_NO_MSG(m_DescriptorCount < m_Capacity);

	const uint32 index = m_FreeHandles[m_DescriptorCount];
	const uint32 offset = index * m_DescriptorSize;
	++m_DescriptorCount;

#ifdef RS_CONFIG_DEBUG
	RS_ASSERT(index != INVALID_HANDLE_INDEX, "Trying to allocate a descriptor with an invalid index! Resource: {}", m_DebugName.c_str());
#endif

	Dx12DescriptorHandle handle;
	handle.m_Cpu.ptr = m_CpuStart.ptr + offset;
	if (IsShaderVisible())
	{
		handle.m_Gpu.ptr = m_GpuStart.ptr + offset;
	}

#ifdef RS_CONFIG_DEBUG
	handle.m_Container = this;
	handle.m_Index = index;
#endif

	return handle;
}

void RS::DX12::Dx12DescriptorHeap::Free(Dx12DescriptorHandle& handle)
{
	if (!handle.IsValid())
	{
		LOG_WARNING("Trying to free an invalid descriptor handle!");
		return;
	}

	std::lock_guard lock{ m_Mutex };
	RS_ASSERT_NO_MSG(m_Heap && m_DescriptorCount);
	RS_ASSERT_NO_MSG(handle.m_Container == this);
	RS_ASSERT_NO_MSG(handle.m_Cpu.ptr >= m_CpuStart.ptr);
	RS_ASSERT_NO_MSG((handle.m_Cpu.ptr - m_CpuStart.ptr) % m_DescriptorSize == 0);
	RS_ASSERT_NO_MSG(handle.m_Index < m_Capacity);
	const uint32 index = (uint32)(handle.m_Cpu.ptr - m_CpuStart.ptr) / m_DescriptorSize;
	RS_ASSERT_NO_MSG(handle.m_Index == index);

	const uint32 frameIndex = Dx12Core2::Get()->GetCurrentFrameIndex();
	m_DeferredFreeIndices[frameIndex].push_back(index);
	Dx12Core2::Get()->SetDeferredReleasesFlag(frameIndex);
	handle = {};
}

void RS::DX12::Dx12DescriptorHeap::ProcessDeferredFree(uint32 frameIndex)
{
	std::lock_guard lock{ m_Mutex };
	RS_ASSERT_NO_MSG(frameIndex < FRAME_BUFFER_COUNT);

	std::vector<uint32>& indices = m_DeferredFreeIndices[frameIndex];
	if (!indices.empty())
	{
		for (uint32 index : indices)
		{
#ifdef RS_CONFIG_DEBUG
			// Ensure that the index is in the list.
			bool valid = ValidateFree(index);
			if (valid)
				LOG_WARNING("Trying to free a descriptor with an index of {} which is not avaliable! Resource: {}", index, m_DebugName.c_str());
#endif

			--m_DescriptorCount;
			m_FreeHandles[m_DescriptorCount] = index;
		}
		indices.clear();
	}
}

#ifdef RS_CONFIG_DEBUG
bool RS::DX12::Dx12DescriptorHeap::ValidateFree(uint32 handleIndex) const
{
	bool found = false;
	for (uint32 i = 0; i < m_DescriptorCount; i++)
	{
		if (handleIndex == m_FreeHandles[i])
		{
			m_FreeHandles[i] = INVALID_HANDLE_INDEX;
			found = true;
			break;
		}
	}
	return found;
}
#endif

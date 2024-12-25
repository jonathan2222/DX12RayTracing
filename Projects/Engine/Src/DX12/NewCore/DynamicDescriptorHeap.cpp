#include "PreCompiled.h"
#include "DynamicDescriptorHeap.h"

#include "DX12/NewCore/DX12Core3.h"
#include "CommandList.h"
#include "RootSignature.h"

RS::DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 numDescriptorsPerHeap)
	: m_DescriptorHeapType(heapType)
	, m_NumDescriptorsPerHeap(numDescriptorsPerHeap)
	, m_DescriptorTableBitMask(0)
	, m_StaleDescriptorTableBitMask(0)
	, m_CurrentCPUDescriptorHandle(D3D12_DEFAULT)
	, m_CurrentGPUDescriptorHandle(D3D12_DEFAULT)
	, m_NumFreeHandles(0)
{
	m_DescriptorHandleIncrementSize = DX12Core3::Get()->GetD3D12Device()->GetDescriptorHandleIncrementSize(heapType);

	// Allocate space for CPU visible descriptors.
	m_DescriptorHandleCache = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(m_NumDescriptorsPerHeap);
}

RS::DynamicDescriptorHeap::~DynamicDescriptorHeap()
{
	Reset(); // Should this be done here?
}

void RS::DynamicDescriptorHeap::StageDescriptors(uint32 rootParameterIndex, uint32_t offset, uint32_t numDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor)
{
	// Cannot stage more than the maximum number of descriptors per heap.
	// Cannot stage more than MaxDescriptorTables root parameters.
	if (numDescriptors > m_NumDescriptorsPerHeap || rootParameterIndex >= MaxDescriptorTables)
	{
		throw std::bad_alloc();
	}

	DescriptorTableCache& descriptorTableCache = m_DescriptorTableCache[rootParameterIndex];

	// Check that the number of descriptors to copy does not exceed the number of descriptors expected in the descriptor table.
	if ((offset + numDescriptors) > descriptorTableCache.numDescriptors)
	{
		throw std::length_error("Number of descriptors exceeds the number of descriptors in the descriptor table.");
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* dstDescriptor = (descriptorTableCache.baseDescriptor + offset);
	for (uint32_t i = 0; i < numDescriptors; ++i)
	{
		dstDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(srcDescriptor, i, m_DescriptorHandleIncrementSize);
	}

	AddDescriptorRange(rootParameterIndex, offset, numDescriptors);

	// Set the root parameter index bit to make sure the descriptor table at that index is bound to the command list.
	m_StaleDescriptorTableBitMask |= (1 << rootParameterIndex);
}

void RS::DynamicDescriptorHeap::CommitStagedDescriptors(CommandList& commandList, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc, bool onlyCopyStagedDescriptors)
{
	// Compute the number of descriptors that need to be copied 
	uint32 numDescriptorsToCommit = ComputeStaleDescriptorCount();

	if (numDescriptorsToCommit > 0)
	{
		auto device = DX12Core3::Get()->GetD3D12Device();
		auto d3d12GraphicsCommandList = commandList.GetGraphicsCommandList().Get();
		RS_ASSERT_NO_MSG(d3d12GraphicsCommandList != nullptr);

		if (!m_CurrentDescriptorHeap || m_NumFreeHandles < numDescriptorsToCommit)
		{
			m_CurrentDescriptorHeap = RequestDescriptorHeap();
			m_CurrentCPUDescriptorHandle = m_CurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			m_CurrentGPUDescriptorHandle = m_CurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
			m_NumFreeHandles = m_NumDescriptorsPerHeap;

			commandList.SetDescriptorHeap(m_DescriptorHeapType, m_CurrentDescriptorHeap.Get());

			// When updating the descriptor heap on the command list, all descriptor
			//  tables must be (re)recopied to the new descriptor heap (not just the stale descriptor tables).
			m_StaleDescriptorTableBitMask = m_DescriptorTableBitMask;
		}

		DWORD rootIndex = 0;
		// Scan from LSB to MSB for a bit set in staleDescriptorsBitMask
		while (_BitScanForward(&rootIndex, m_StaleDescriptorTableBitMask))
		{
			uint32 numSrcDescriptors = m_DescriptorTableCache[rootIndex].numDescriptors;
			D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorHandles = m_DescriptorTableCache[rootIndex].baseDescriptor;

			if (onlyCopyStagedDescriptors)
			{
				// Within this table, only copy the descriptors that was staged.
				std::vector<DescriptorRange> ranges = m_DescriptorRangesPerTable[rootIndex];
				for (uint32 i = 0; i < ranges.size(); ++i)
				{
					const DescriptorRange& range = ranges[i];

					D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorHanldesForRange = pSrcDescriptorHandles + range.offset;

					CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle(m_CurrentCPUDescriptorHandle);
					cpuDescriptorHandle.Offset(range.offset, m_DescriptorHandleIncrementSize);

					D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[] = { cpuDescriptorHandle };
					uint32 pDestDescriptorRangeSizes[] = { range.numDescriptors };

					device->CopyDescriptors(1, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes, range.numDescriptors, pSrcDescriptorHanldesForRange, nullptr, m_DescriptorHeapType);
				}
				ranges.clear();
			}
			else
			{
				D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[] = { m_CurrentCPUDescriptorHandle };
				uint32 pDestDescriptorRangeSizes[] = { numSrcDescriptors };

				// Copy the staged CPU visible descriptors to the GPU visible descriptor heap.
				// TODO: Only copy descriptor if it was staged! This will fix the sparse binding problem.
				device->CopyDescriptors(1, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes, numSrcDescriptors, pSrcDescriptorHandles, nullptr, m_DescriptorHeapType);
			}

			// Set the descriptors on the command list using the passed-in setter function.
			setFunc(d3d12GraphicsCommandList, rootIndex, m_CurrentGPUDescriptorHandle);

			// Offset current CPU and GPU descriptor handles to the next table.
			m_CurrentCPUDescriptorHandle.Offset(numSrcDescriptors, m_DescriptorHandleIncrementSize);
			m_CurrentGPUDescriptorHandle.Offset(numSrcDescriptors, m_DescriptorHandleIncrementSize);
			m_NumFreeHandles -= numSrcDescriptors;

			// Flip the stale bit so the descriptor table is not recopied again unless it is updated with a new descriptor.
			m_StaleDescriptorTableBitMask ^= (1 << rootIndex);
		}
	}
}

void RS::DynamicDescriptorHeap::CommitStagedDescriptorsForDraw(CommandList& commandList)
{
	CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable, true);
}

void RS::DynamicDescriptorHeap::CommitStagedDescriptorsForDispatch(CommandList& commandList)
{
	CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable, true);
}

D3D12_GPU_DESCRIPTOR_HANDLE RS::DynamicDescriptorHeap::CopyDescriptor(CommandList& comandList, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor)
{
	if (!m_CurrentDescriptorHeap || m_NumFreeHandles < 1)
	{
		m_CurrentDescriptorHeap = RequestDescriptorHeap();
		m_CurrentCPUDescriptorHandle = m_CurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_CurrentGPUDescriptorHandle = m_CurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		m_NumFreeHandles = m_NumDescriptorsPerHeap;

		comandList.SetDescriptorHeap(m_DescriptorHeapType, m_CurrentDescriptorHeap.Get());

		// When updating the descriptor heap on the command list, all descriptor
		// tables must be (re)recopied to the new descriptor heap (not just the stale descriptor tables).
		m_StaleDescriptorTableBitMask = m_DescriptorTableBitMask;
	}

	auto device = DX12Core3::Get()->GetD3D12Device();

	D3D12_GPU_DESCRIPTOR_HANDLE hGPU = m_CurrentGPUDescriptorHandle;
	device->CopyDescriptorsSimple(1, m_CurrentCPUDescriptorHandle, cpuDescriptor, m_DescriptorHeapType);

	m_CurrentCPUDescriptorHandle.Offset(1, m_DescriptorHandleIncrementSize);
	m_CurrentGPUDescriptorHandle.Offset(1, m_DescriptorHandleIncrementSize);
	m_NumFreeHandles -= 1;

	return hGPU;
}

void RS::DynamicDescriptorHeap::ParseRootSignature(const std::shared_ptr<RootSignature>& pRootSignature)
{
	// If the root signature changes, all descriptors must be (re)bound to the command list.
	m_StaleDescriptorTableBitMask = 0;

	const auto& rootSignatureDesc = pRootSignature->GetRootSignatureDesc();

	// Get a bit mask that represents the root parameter indices that match the descriptor heap type for this dynamic descriptor heap.
	m_DescriptorTableBitMask = pRootSignature->GetDescriptorTableBitMask(m_DescriptorHeapType);
	uint32 descriptorTableBitMask = m_DescriptorTableBitMask;

	uint32 currentOffset = 0;
	DWORD rootIndex = 0;
	while (_BitScanForward(&rootIndex, descriptorTableBitMask) && rootIndex < rootSignatureDesc.Desc_1_1.NumParameters)
	{
		uint32 numDescriptors = pRootSignature->GetNumDescriptors(rootIndex);

		DescriptorTableCache& descriptorTableCache = m_DescriptorTableCache[rootIndex];
		descriptorTableCache.numDescriptors = numDescriptors;
		descriptorTableCache.baseDescriptor = m_DescriptorHandleCache.get() + currentOffset;

		currentOffset += numDescriptors;

		// Flip the descriptor table bit so it's not scanned again for the current index.
		descriptorTableBitMask ^= (1 << rootIndex);
	}

	// Make sure the maximum number of descriptors per descriptor heap has not been exceeded.
	RS_ASSERT(currentOffset <= m_NumDescriptorsPerHeap, "The root signature requires more than the maximum number of descriptors per descriptor heap. Consider increasing the maximum number of descriptors per descriptor heap.");
}

void RS::DynamicDescriptorHeap::Reset()
{
	m_AvailableDescriptorHeaps = m_DescriptorHeapPool;
	m_CurrentDescriptorHeap.Reset();
	m_CurrentCPUDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
	m_CurrentGPUDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
	m_NumFreeHandles = 0;
	m_DescriptorTableBitMask = 0;
	m_StaleDescriptorTableBitMask = 0;

	// Reset the table cache
	for (int i = 0; i < MaxDescriptorTables; ++i)
	{
		m_DescriptorTableCache[i].Reset();
		m_DescriptorRangesPerTable[i].clear();
	}
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RS::DynamicDescriptorHeap::RequestDescriptorHeap()
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	if (!m_AvailableDescriptorHeaps.empty())
	{
		descriptorHeap = m_AvailableDescriptorHeaps.front();
		m_AvailableDescriptorHeaps.pop();
	}
	else
	{
		descriptorHeap = CreateDescriptorHeap();
		m_DescriptorHeapPool.push(descriptorHeap);
	}

	return descriptorHeap;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RS::DynamicDescriptorHeap::CreateDescriptorHeap()
{
	auto device = DX12Core3::Get()->GetD3D12Device();

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = m_DescriptorHeapType;
	descriptorHeapDesc.NumDescriptors = m_NumDescriptorsPerHeap;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	DXCall(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

uint32 RS::DynamicDescriptorHeap::ComputeStaleDescriptorCount() const
{
	uint32 numStaleDescriptors = 0;
	DWORD i = 0;
	DWORD staleDescriptorsBitMask = m_StaleDescriptorTableBitMask;

	while (_BitScanForward(&i, staleDescriptorsBitMask))
	{
		numStaleDescriptors += m_DescriptorTableCache[i].numDescriptors;
		staleDescriptorsBitMask ^= (1 << i);
	}

	return numStaleDescriptors;
}

void RS::DynamicDescriptorHeap::AddDescriptorRange(uint32 rootParameterIndex, uint32_t offset, uint32_t numDescriptors)
{
	bool addedRange = false;
	uint32 afterIndex = UINT32_MAX;
	std::vector<DescriptorRange>& ranges = m_DescriptorRangesPerTable[rootParameterIndex];
	uint32 numRanges = ranges.size();
	uint32 rangeIndex = 0;
	while (rangeIndex < numRanges)
	{
		DescriptorRange* pPreRange = rangeIndex != 0 ? &ranges[rangeIndex - 1] : nullptr;

		DescriptorRange& range = ranges[rangeIndex];
		// Merge ranges if overlap. (Should most likely not occur, think it should actually result in an error)
		uint32 startA = offset;									// !
		uint32 endA = offset + numDescriptors - 1;				// !
		uint32 startB = range.offset;							// |
		uint32 endB = range.offset + range.numDescriptors - 1;	// |
		// -----------|---!--|--!------------
		// --------!--|---!--|---------------
		// -----------|-!-!--|---------------
		// ---------!-|------|-!-------------
		if ((startA >= startB && startA < endB) || (endA > startB && endA < endB))
		{
			// Descriptor ranges should be able to overlap if they are the exact same range position and size, and if they have the same type?
			// Might actually only need to be the same type!
			RS_ASSERT(startA == startB && endA == endB, "Cannot overlap decriptor ranges if not the exact same!");
			//RS_ASSERT(false, "Cannot overlap descriptors!");
			range.offset = std::min(range.offset, offset);
			range.numDescriptors = std::max(endA, endB) - range.offset;
			break;
		}

		// Merge if adjecent
		// (1) -----------|------|!----!---------
		if (endB + 1 == startA)
		{
			range.numDescriptors += numDescriptors;
			addedRange = true;
		}

		// (2) -----!----!|------|---------------
		if (endA + 1 == range.offset)
		{
			range.offset = offset;
			range.numDescriptors += numDescriptors;
			addedRange = true;
		}

		if (range.offset + range.numDescriptors < offset)
			afterIndex = rangeIndex;

		// Remove current range and merge if next to previous.
		if (pPreRange)
		{
			if (pPreRange->offset + pPreRange->numDescriptors - 1 == range.offset)
			{
				pPreRange->numDescriptors += range.numDescriptors;
				ranges.erase(ranges.begin() + rangeIndex);
				rangeIndex--;
			}
		}

		rangeIndex++;
	}

	// We add it if we did not merge the new range.
	if (!addedRange)
	{
		if (afterIndex != UINT32_MAX)
			ranges.insert(ranges.begin() + afterIndex, DescriptorRange(offset, numDescriptors));
		else
			ranges.emplace_back(offset, numDescriptors);
	}
}

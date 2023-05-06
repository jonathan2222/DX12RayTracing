#include "PreCompiled.h"
#include "Dx12Resources.h"

#include <d3d12.h>
#include "DX12Defines.h"
#include "Dx12Core2.h"

#ifdef RS_CONFIG_DEBUG
void RS::DX12::Dx12DescriptorHandle::SetDebugName(const std::string& name)
{
	m_Container->m_DescriptorNames[m_FreeIndex] = name;
}
#endif

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
	m_DescriptorNames.resize(m_Capacity, "");
#endif

	m_DescriptorSize = pDevice->GetDescriptorHandleIncrementSize(m_Type);
	m_CpuStart = m_Heap->GetCPUDescriptorHandleForHeapStart();
	m_GpuStart = isShaderVisible ? m_Heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
}

void RS::DX12::Dx12DescriptorHeap::Release()
{
#ifdef RS_CONFIG_DEBUG
	if (m_DescriptorCount > 0)
	{
		LOG_CRITICAL("Descriptors are still left in the heap ({}). You might have forgotten to call Free on the descriptor handle?", m_DebugName.c_str());
		std::string str;
		for (uint32 i = 0; i < m_DescriptorCount; ++i)
		{
			if (i == 0)
				str += '\n';
			str += "->\t" + m_DescriptorNames[i] + " (" + std::to_string(m_FreeHandles[i]) + ")\n";
		}
		LOG_CRITICAL("Descriptor indices: {}", str.c_str());
		LOG_FLUSH();
	}
#endif
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
	handle.m_FreeIndex = m_DescriptorCount - 1;
	m_DescriptorNames[handle.m_FreeIndex] = "Occupied";
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
			if (!valid)
				LOG_WARNING("Trying to free a descriptor with an index of {} which is not avaliable! Resource: {}", index, m_DebugName.c_str());
			m_DescriptorNames[m_DescriptorCount - 1] = "";
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

void RS::DX12::Dx12VertexBuffer::Create(uint8* pInitialData, uint32 stride, uint32 size, const char* debugName)
{
	ID3D12Device* const pDevice = Dx12Core2::Get()->GetD3D12Device();
	RS_ASSERT_NO_MSG(pDevice);

	// TODO: Change to using a default heap and upload the data to it using a upload heap. Can be separate function?
	DXCall(pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pResource)));
	DX12_SET_DEBUG_NAME(pResource, debugName ? debugName : "Vertex buffer");

	// Copy the vertex data to the vertex buffer.
	UINT8* pDataBegin;
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	DXCall(pResource->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
	memcpy(pDataBegin, pInitialData, size);
	pResource->Unmap(0, nullptr);
	
	// Initialize the vertex buffer view.
	view.BufferLocation = pResource->GetGPUVirtualAddress();
	view.StrideInBytes = stride;
	view.SizeInBytes = size;
}

void RS::DX12::Dx12VertexBuffer::Release()
{
	Dx12Core2::Get()->DeferredRelease(pResource);
	view.BufferLocation = 0;
	view.SizeInBytes = 0;
	view.StrideInBytes = 0;
}

void RS::DX12::Dx12VertexBuffer::Map()
{
	LOG_ERROR("Map is not implemented!");
}

void RS::DX12::Dx12VertexBuffer::Unmap()
{
	LOG_ERROR("Map is not implemented!");
}

void RS::DX12::Dx12Buffer::Create(uint8* pInitialData, uint32 size, const char* debugName)
{
	m_Size = size;

	// If using placed resources
	//m_AlignedSize = Utils::AlignUp(size, (uint32)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

	ID3D12Device* const pDevice = Dx12Core2::Get()->GetD3D12Device();
	RS_ASSERT_NO_MSG(pDevice);

	DXCall(pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(Utils::AlignUp(m_Size, (uint64)1024*64)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, // we do not have use an optimized clear value for constant buffers
		IID_PPV_ARGS(&pUploadHeap)));

	UINT8* pDataBegin;
	CD3DX12_RANGE readRange(0, 0);
	DXCall(pUploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
	memcpy(pDataBegin, pInitialData, size);
	pUploadHeap->Unmap(0, nullptr);
	DX12_SET_DEBUG_NAME_REF(pUploadHeap, m_DebugName, debugName ? debugName : "CB Upload heap");
}

void RS::DX12::Dx12Buffer::Release()
{
	Dx12Core2::Get()->GetDescriptorHeapGPUResources()->Free(m_Handle);
	Dx12Core2::Get()->DeferredRelease(pUploadHeap);
}

void RS::DX12::Dx12Buffer::CreateView()
{
	ID3D12Device* const pDevice = Dx12Core2::Get()->GetD3D12Device();
	RS_ASSERT_NO_MSG(pDevice);

	//D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	//uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	//uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	//uavDesc.Buffer.CounterOffsetInBytes = 0;
	//uavDesc.Buffer.FirstElement = 0;
	//uavDesc.Buffer.StructureByteStride = m_Stride;
	//uavDesc.Buffer.NumElements = m_Size / m_Stride;
	//pDevice->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, handle.m_Cpu);

	m_Handle = Dx12Core2::Get()->GetDescriptorHeapGPUResources()->Allocate();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = pUploadHeap->GetGPUVirtualAddress();
	// Constant buffer size is required to be 256-byte aligned.
	cbvDesc.SizeInBytes = Utils::AlignUp(m_Size, (uint64)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	pDevice->CreateConstantBufferView(&cbvDesc, m_Handle.m_Cpu);

#ifdef RS_CONFIG_DEBUG
	m_Handle.SetDebugName(m_DebugName);
#endif
}

void RS::DX12::Dx12Buffer::Map()
{
	LOG_ERROR("Not implemented");
}

void RS::DX12::Dx12Buffer::Unmap()
{
	LOG_ERROR("Not implemented");
}

void RS::DX12::Dx12Texture::Create(uint8* pInitialData, uint32 width, uint32 height, DXGI_FORMAT format, const char* debugName)
{
	m_Format = format;

	ID3D12Device* const pDevice = Dx12Core2::Get()->GetD3D12Device();
	RS_ASSERT_NO_MSG(pDevice);

	// Texture destination resource.
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = (UINT64)width;
	resourceDesc.Height = height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	{ // TODO: Make it possible to use placed heaps.
		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;

		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST;

		//D3D12_CLEAR_VALUE clearValue{};
		//float pinkColor[4] = { 1.0, 0.0, 1.0, 1.0 };
		//memcpy(clearValue.Color, pinkColor, sizeof(float) * 4);
		//clearValue.Format = format;
		//clearValue.DepthStencil.Depth = 0.0; // Think about these values more...
		//clearValue.DepthStencil.Stencil = 0.0;
		DXCall(pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			initialState,
			nullptr,
			IID_PPV_ARGS(&pResource)));
		DX12_SET_DEBUG_NAME_REF(pResource, m_DebugName, debugName ? debugName : "Texture resource");
	}

	// Texture Upload resource
	{
		const uint32 subresourceCount = resourceDesc.DepthOrArraySize * resourceDesc.MipLevels;
		const uint64 uploadBufferStep = GetRequiredIntermediateSize(pResource, 0, subresourceCount);
		const uint64 uploadBufferSize = uploadBufferStep;

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
		DXCall(pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&uploadDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&pUploadHeap)));
		DX12_SET_DEBUG_NAME(pUploadHeap, "{} Upload Heap", debugName ? debugName : "Texture resource");

		uint32 numChannels = GetChannelCountFromFormat(format);

		// Copy data to the intermediate upload heap and then schedule a copy from the upload heap to the texture resource.
		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = pInitialData;
		textureData.RowPitch = static_cast<LONG_PTR>(numChannels * resourceDesc.Width);
		textureData.SlicePitch = textureData.RowPitch * resourceDesc.Height;

		Dx12FrameCommandList* pFrameCommandList = Dx12Core2::Get()->GetFrameCommandList();
		ID3D12GraphicsCommandList* pCommandList = pFrameCommandList->GetCommandList();
		UpdateSubresources(pCommandList, pResource, pUploadHeap, 0, 0, subresourceCount, &textureData);

		// Assume use in shader.
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pResource,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		pCommandList->ResourceBarrier(1, &barrier);

		// TODO: Remove this, we do not want to flush every time we upload data!
		pFrameCommandList->Flush();
	}
}

void RS::DX12::Dx12Texture::Release()
{
	Dx12Core2::Get()->GetDescriptorHeapGPUResources()->Free(m_Handle);
	Dx12Core2::Get()->DeferredRelease(pResource);
	Dx12Core2::Get()->DeferredRelease(pUploadHeap);
}

void RS::DX12::Dx12Texture::CreateView()
{
	ID3D12Device* const pDevice = Dx12Core2::Get()->GetD3D12Device();
	RS_ASSERT_NO_MSG(pDevice);

	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Format = m_Format;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Texture2D.MipLevels = 1;
	desc.Texture2D.MostDetailedMip = 0;
	desc.Texture2D.PlaneSlice = 0;
	desc.Texture2D.ResourceMinLODClamp = 0.0f; // Allow access of all mipmap levels.

	m_Handle = Dx12Core2::Get()->GetDescriptorHeapGPUResources()->Allocate();

	pDevice->CreateShaderResourceView(pResource, &desc, m_Handle.m_Cpu);

#ifdef RS_CONFIG_DEBUG
	m_Handle.SetDebugName(m_DebugName);
#endif
}

void RS::DX12::Dx12Texture::Map()
{
	LOG_ERROR("Not implemented");
}

void RS::DX12::Dx12Texture::Unmap()
{
	LOG_ERROR("Not implemented");
}

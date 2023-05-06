#pragma once

#include <mutex>
#include "Dx12Device.h"

namespace RS::DX12
{
	class Dx12DescriptorHeap;
	struct Dx12DescriptorHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE		m_Cpu{};
		D3D12_GPU_DESCRIPTOR_HANDLE		m_Gpu{};

		constexpr bool IsValid() const { return m_Cpu.ptr != 0; }
		constexpr bool IsShaderVisible() const { return m_Gpu.ptr != 0; }
		constexpr uint32 GetIndex() const { return m_Index; }

#ifdef RS_CONFIG_DEBUG
	private:
		friend class Dx12DescriptorHeap;
		Dx12DescriptorHeap* m_Container{ nullptr };
		uint32				m_Index{ UINT32_MAX };
#endif
	};

	// Note: Remember that this heap might be called from different threads!
	class Dx12DescriptorHeap
	{
	public:
		RS_NO_COPY_AND_MOVE(Dx12DescriptorHeap);
		Dx12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) : m_Type(type) {}
		~Dx12DescriptorHeap() = default;

		void Init(uint32 capacity, bool isShaderVisible, const std::string name = "");
		void Release();

		[[nodiscard]] Dx12DescriptorHandle Allocate();
		void Free(Dx12DescriptorHandle& handle);
		void ProcessDeferredFree(uint32 frameIndex);

		constexpr D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_Type; }
		constexpr D3D12_CPU_DESCRIPTOR_HANDLE GetCpuStart() const { return m_CpuStart; }
		constexpr D3D12_GPU_DESCRIPTOR_HANDLE GetGpuStart() const { return m_GpuStart; }
		constexpr ID3D12DescriptorHeap* const GetHeap() const { return m_Heap; }
		constexpr uint32 GetCapacity() const { return m_Capacity; }
		constexpr uint32 GetDescriptorCount() const { return m_DescriptorCount; }
		constexpr uint32 GetDescriptorSize() const { return m_DescriptorSize; }
		constexpr bool IsShaderVisible() const { return m_GpuStart.ptr != 0; }

	private:
#ifdef RS_CONFIG_DEBUG
		const uint32 INVALID_HANDLE_INDEX = UINT32_MAX - 1;
		bool ValidateFree(uint32 handleIndex) const;
#endif

	private:
		ID3D12DescriptorHeap*				m_Heap = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE			m_CpuStart{};
		D3D12_GPU_DESCRIPTOR_HANDLE			m_GpuStart{};
		std::unique_ptr<uint32[]>			m_FreeHandles{}; // Holds free handles AFTER index [m_DescriptorCount-1]!
		std::vector<uint32>					m_DeferredFreeIndices[FRAME_BUFFER_COUNT]{};
		uint32								m_Capacity = 0;
		uint32								m_DescriptorCount = 0;
		uint32								m_DescriptorSize = 0;
		const D3D12_DESCRIPTOR_HEAP_TYPE	m_Type;
		std::mutex							m_Mutex;

#ifdef RS_CONFIG_DEBUG
		std::string							m_DebugName = "Unnamed Heap"; // Name of the heap
#endif
	};

	class Dx12VertexBuffer
	{
	public:
		void Create(uint8* pInitialData, uint32 stride, uint32 size);
		void Release();

		void Map();
		void Unmap();

		D3D12_VERTEX_BUFFER_VIEW view;
		ID3D12Resource* pResource = nullptr;
	};

	// Note: A buffer should be able to update as another is used in a frame. TODO: Do something about this.
	// TODO: Make another one for structured buffers.
	class Dx12Buffer
	{
	public:
		void Create(uint8* pInitialData, uint32 size);
		void Release();

		void CreateView(Dx12DescriptorHandle handle);

		void Map();
		void Unmap();

		Dx12DescriptorHandle handle;
		ID3D12Resource* pUploadHeap = nullptr;

		uint64 m_Size;
	};

	class Dx12Texture
	{
	public:
		// TODO: Implement this!
		// Might want to do a separate texture class for textures on a render target.
		void Create(uint8* pInitialData, uint32 width, uint32 height, DXGI_FORMAT format);
		void Release();

		void CreateView(Dx12DescriptorHandle handle);

		void Map();
		void Unmap();

		uint32 GetDescriptorIndex() const { return m_Handle.GetIndex(); }

		ID3D12Resource* pResource = nullptr;
		ID3D12Resource* pUploadHeap = nullptr;

	private:
		DXGI_FORMAT m_Format = DXGI_FORMAT_UNKNOWN;
		Dx12DescriptorHandle m_Handle;
	};
}
#include "PreCompiled.h"
#include "UploadBuffer.h"

#include "DX12/NewCore/DX12Core3.h"
#include <new> // std::bad_alloc

RS::UploadBuffer::UploadBuffer(size_t pageSize)
    : m_PageSize(pageSize)
{
}

RS::UploadBuffer::Allocation RS::UploadBuffer::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (sizeInBytes > m_PageSize)
        throw std::bad_alloc();

    // If there is no current page, or the requested allocation exceeds the remaining space in the current page, request a new page.
    if (!m_CurrentPage || !m_CurrentPage->HasSpace(sizeInBytes, alignment))
        m_CurrentPage = RequestPage();

    return m_CurrentPage->Allocate(sizeInBytes, alignment);
}

void RS::UploadBuffer::Reset()
{
    m_CurrentPage = nullptr;

    // Reset all available pages.
    m_AvailablePages = m_PagePool;

    for (auto page : m_AvailablePages)
    {
        // Reset the page for new allocations.
        page->Reset();
    }
}

std::shared_ptr<RS::UploadBuffer::Page> RS::UploadBuffer::RequestPage()
{
    std::shared_ptr<Page> page;

    if (!m_AvailablePages.empty())
    {
        page = m_AvailablePages.front();
        m_AvailablePages.pop_front();
    }
    else
    {
        page = std::make_shared<Page>(m_PageSize);
        m_PagePool.push_back(page);
    }

    return page;
}

RS::UploadBuffer::Page::Page(size_t sizeInBytes)
    : m_PageSize(sizeInBytes)
    , m_Offset(0)
    , m_CPUPtr(nullptr)
    , m_GPUPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
    auto device = DX12Core3::Get()->GetD3D12Device();
    DXCall(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(m_PageSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_d3d12Resource)
    ));

    m_GPUPtr = m_d3d12Resource->GetGPUVirtualAddress();
    DXCall(m_d3d12Resource->Map(0, nullptr, &m_CPUPtr)); // Because it is an upload heap we can leave it mapped.
}

RS::UploadBuffer::Page::~Page()
{
    m_d3d12Resource->Unmap(0, nullptr);
    m_CPUPtr = nullptr;
    m_GPUPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool RS::UploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
    size_t alignedSize = Utils::AlignUp<size_t>(sizeInBytes, alignment);
    size_t alignedOffset = Utils::AlignUp<size_t>(m_Offset, alignment);
    return alignedOffset + alignedSize <= m_PageSize;
}

RS::UploadBuffer::Allocation RS::UploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (!HasSpace(sizeInBytes, alignment))
    {
        // Can't allocate space from page.
        throw std::bad_alloc();
    }

    size_t alignedSize = Utils::AlignUp<size_t>(sizeInBytes, alignment);
    m_Offset = Utils::AlignUp<size_t>(m_Offset, alignment);

    Allocation allocation;
    allocation.CPU = static_cast<uint8*>(m_CPUPtr) + m_Offset;
    allocation.GPU = m_GPUPtr + m_Offset;

    m_Offset += alignedSize;

    return allocation;
}

void RS::UploadBuffer::Page::Reset()
{
    m_Offset = 0;
}

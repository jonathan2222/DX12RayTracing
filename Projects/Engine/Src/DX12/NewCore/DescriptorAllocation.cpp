#include "PreCompiled.h"
#include "DescriptorAllocation.h"

#include "Core/EngineLoop.h" // Frame number
#include "DescriptorAllocatorPage.h"

RS::DescriptorAllocation::DescriptorAllocation()
    : m_Descriptor{0}
    , m_NumHandles(0)
    , m_DescriptorSize(0)
    , m_Page(nullptr)
{
}

RS::DescriptorAllocation::DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32 numHandles, uint32 descriptorSize, std::shared_ptr<DescriptorAllocatorPage> page)
    : m_Descriptor(descriptor)
    , m_NumHandles(numHandles)
    , m_DescriptorSize(descriptorSize)
    , m_Page(page)
{
}

RS::DescriptorAllocation::~DescriptorAllocation()
{
    Free();
}

RS::DescriptorAllocation::DescriptorAllocation(DescriptorAllocation&& allocation)
    : m_Descriptor(allocation.m_Descriptor)
    , m_NumHandles(allocation.m_NumHandles)
    , m_DescriptorSize(allocation.m_DescriptorSize)
    , m_Page(allocation.m_Page)
{
    allocation.m_Descriptor.ptr = 0;
    allocation.m_NumHandles     = 0;
    allocation.m_DescriptorSize = 0;
}

RS::DescriptorAllocation& RS::DescriptorAllocation::operator=(DescriptorAllocation&& other)
{
    // Free this descriptor if it points to anything.
    Free();

    m_Descriptor            = other.m_Descriptor;
    m_NumHandles            = other.m_NumHandles;
    m_DescriptorSize        = other.m_DescriptorSize;
    m_Page                  = std::move(other.m_Page);

    other.m_Descriptor.ptr  = 0;
    other.m_NumHandles      = 0;
    other.m_DescriptorSize  = 0;

    return *this;
}

bool RS::DescriptorAllocation::IsNull() const
{
    return m_Descriptor.ptr == 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE RS::DescriptorAllocation::GetDescriptorHandle(uint32 offset) const
{
    RS_ASSERT(offset < m_NumHandles);
    return { m_Descriptor.ptr + (m_DescriptorSize * offset) };
}

uint32 RS::DescriptorAllocation::GetNumHandles() const
{
    return m_NumHandles;
}

std::shared_ptr<RS::DescriptorAllocatorPage> RS::DescriptorAllocation::GetDescriptorAllocatorPage() const
{
    return m_Page;
}

void RS::DescriptorAllocation::Free()
{
    if (!IsNull() && m_Page)
    {
        m_Page->Free(std::move(*this), EngineLoop::GetCurrentFrameNumber());

        m_Descriptor.ptr    = 0;
        m_NumHandles        = 0;
        m_DescriptorSize    = 0;
        m_Page.reset();
    }
}

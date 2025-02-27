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
// Modified by: Jonathan �leskog 

#include "PreCompiled.h"
#include "DXGPUBuffers.h"
//#include "pch.h"
//#include "GraphicsCore.h"
//#include "EsramAllocator.h"
#include "DXCommandContext.h"
//#include "BufferManager.h"
//#include "UploadBuffer.h"

#include "DX12/Final/DXCore.h"

//using namespace Graphics;
namespace RS::DX12
{
    void DXGPUBuffer::Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, const void* initialData, D3D12_HEAP_TYPE heapType)
    {
        Destroy();

        m_ElementCount = NumElements;
        m_ElementSize = ElementSize;
        m_BufferSize = NumElements * ElementSize;

        if (heapType == D3D12_HEAP_TYPE_UPLOAD)
            m_ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
        D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();

        m_UsageState = D3D12_RESOURCE_STATE_COMMON;

        D3D12_HEAP_PROPERTIES HeapProps;
        HeapProps.Type = heapType;
        HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProps.CreationNodeMask = 1;
        HeapProps.VisibleNodeMask = 1;

        DXCall(DXCore::GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
            &ResourceDesc, m_UsageState, nullptr, IID_PPV_ARGS(&m_pResource)));
        m_Alive = true;

        m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

        if (initialData)
            DXCommandContext::InitializeBuffer(*this, initialData, m_BufferSize);

#ifdef RS_CONFIG_DEVELOPMENT
        m_pResource->SetName(name.c_str());
#else
        RS_UNUSED(name);
#endif

        CreateDerivedViews();
    }

    void DXGPUBuffer::Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, const DXUploadBuffer& srcData, uint32_t srcOffset, D3D12_HEAP_TYPE heapType)
    {
        Destroy();

        m_ElementCount = NumElements;
        m_ElementSize = ElementSize;
        m_BufferSize = NumElements * ElementSize;

        if (heapType == D3D12_HEAP_TYPE_UPLOAD)
            m_ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
        D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();

        m_UsageState = D3D12_RESOURCE_STATE_COMMON;

        D3D12_HEAP_PROPERTIES HeapProps;
        HeapProps.Type = heapType;
        HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProps.CreationNodeMask = 1;
        HeapProps.VisibleNodeMask = 1;

        DXCall(DXCore::GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                &ResourceDesc, m_UsageState, nullptr, IID_PPV_ARGS(&m_pResource)));
        m_Alive = true;

        m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

        DXCommandContext::InitializeBuffer(*this, srcData, srcOffset);

#ifdef RS_CONFIG_DEVELOPMENT
        m_pResource->SetName(name.c_str());
#else
        RS_UNUSED(name);
#endif

        CreateDerivedViews();
    }

    // Sub-Allocate a buffer out of a pre-allocated heap.  If initial data is provided, it will be copied into the buffer using the default command context.
    void DXGPUBuffer::CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t HeapOffset, uint32_t NumElements, uint32_t ElementSize,
        const void* initialData)
    {
        m_ElementCount = NumElements;
        m_ElementSize = ElementSize;
        m_BufferSize = NumElements * ElementSize;

        D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();

        m_UsageState = D3D12_RESOURCE_STATE_COMMON;

        DXCall(DXCore::GetDevice()->CreatePlacedResource(pBackingHeap, HeapOffset, &ResourceDesc, m_UsageState, nullptr, IID_PPV_ARGS(&m_pResource)));
        m_Alive = true;

        m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

        if (initialData)
            DXCommandContext::InitializeBuffer(*this, initialData, m_BufferSize);

#ifdef RS_CONFIG_DEVELOPMENT
        m_pResource->SetName(name.c_str());
#else
        RS_UNUSED(name);
#endif

        CreateDerivedViews();
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE& DXGPUBuffer::GetUAV(void) const
    {
        RS_ASSERT((m_ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0);
        return m_UAV;
    }

    //void DXGPUBuffer::Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, EsramAllocator& Allocator, const void* initialData)
    //{
    //    (void)Allocator;
    //    Create(name, NumElements, ElementSize, initialData);
    //}

    D3D12_CPU_DESCRIPTOR_HANDLE DXGPUBuffer::CreateConstantBufferView(uint32_t Offset, uint32_t Size) const
    {
        RS_ASSERT(Offset + Size <= m_BufferSize);

        Size = Utils::AlignUp(Size, 16);

        D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
        CBVDesc.BufferLocation = m_GpuVirtualAddress + (size_t)Offset;
        CBVDesc.SizeInBytes = Size;

        D3D12_CPU_DESCRIPTOR_HANDLE hCBV = DXCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        DXCore::GetDevice()->CreateConstantBufferView(&CBVDesc, hCBV);
        return hCBV;
    }

    D3D12_RESOURCE_DESC DXGPUBuffer::DescribeBuffer(void)
    {
        RS_ASSERT(m_BufferSize != 0);

        D3D12_RESOURCE_DESC Desc = {};
        Desc.Alignment = 0;
        Desc.DepthOrArraySize = 1;
        Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags = m_ResourceFlags;
        Desc.Format = DXGI_FORMAT_UNKNOWN;
        Desc.Height = 1;
        Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.MipLevels = 1;
        Desc.SampleDesc.Count = 1;
        Desc.SampleDesc.Quality = 0;
        Desc.Width = (UINT64)m_BufferSize;
        return Desc;
    }

    void DXByteAddressBuffer::CreateDerivedViews(void)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        SRVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
        SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

        if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
            m_SRV = DXCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        DXCore::GetDevice()->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRV);

        if ((m_ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
            UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            UAVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
            UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

            if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
                m_UAV = DXCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            DXCore::GetDevice()->CreateUnorderedAccessView(m_pResource.Get(), nullptr, &UAVDesc, m_UAV);
        }
    }

    void DXStructuredBuffer::CreateDerivedViews(void)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
        SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        SRVDesc.Buffer.NumElements = m_ElementCount;
        SRVDesc.Buffer.StructureByteStride = m_ElementSize;
        SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
            m_SRV = DXCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        DXCore::GetDevice()->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRV);

        D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
        UAVDesc.Buffer.CounterOffsetInBytes = 0;
        UAVDesc.Buffer.NumElements = m_ElementCount;
        UAVDesc.Buffer.StructureByteStride = m_ElementSize;
        UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        m_CounterBuffer.Create(L"StructuredBuffer::Counter", 1, 4);

        if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
            m_UAV = DXCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        DXCore::GetDevice()->CreateUnorderedAccessView(m_pResource.Get(), m_CounterBuffer.GetResource(), &UAVDesc, m_UAV);
    }

    void DXTypedBuffer::CreateDerivedViews(void)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        SRVDesc.Format = m_DataFormat;
        SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        SRVDesc.Buffer.NumElements = m_ElementCount;
        SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
            m_SRV = DXCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        DXCore::GetDevice()->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRV);

        if ((m_ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
            UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            UAVDesc.Format = m_DataFormat;
            UAVDesc.Buffer.NumElements = m_ElementCount;
            UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

            if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
                m_UAV = DXCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            DXCore::GetDevice()->CreateUnorderedAccessView(m_pResource.Get(), nullptr, &UAVDesc, m_UAV);
        }
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE& DXStructuredBuffer::GetCounterSRV(DXCommandContext& Context)
    {
        Context.TransitionResource(m_CounterBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
        return m_CounterBuffer.GetSRV();
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE& DXStructuredBuffer::GetCounterUAV(DXCommandContext& Context)
    {
        Context.TransitionResource(m_CounterBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        return m_CounterBuffer.GetUAV();
    }
}

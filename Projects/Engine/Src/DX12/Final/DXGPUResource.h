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

#pragma once

#include "DX12/Final/DX12Defines.h"

namespace RS::DX12
{
    enum DataAccess
    {
        Read,
        Write,
    };

    class DXGPUResource
    {
        friend class DXCommandContext;
        friend class DXGraphicsContext;
        friend class DXComputeContext;

    public:
        DXGPUResource() :
            m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
            m_UsageState(D3D12_RESOURCE_STATE_COMMON),
            m_TransitioningState((D3D12_RESOURCE_STATES)-1),
            m_Alive(false)
        {
        }

        DXGPUResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
            m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
            m_pResource(pResource),
            m_UsageState(CurrentState),
            m_TransitioningState((D3D12_RESOURCE_STATES)-1),
            m_Alive(true)
        {
        }

        ~DXGPUResource() { Destroy(); }

        virtual void Destroy();

        ID3D12Resource* operator->() { return m_pResource.Get(); }
        const ID3D12Resource* operator->() const { return m_pResource.Get(); }

        ID3D12Resource* GetResource() { return m_pResource.Get(); }
        const ID3D12Resource* GetResource() const { return m_pResource.Get(); }

        ID3D12Resource** GetAddressOf() { return m_pResource.GetAddressOf(); }

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return m_GpuVirtualAddress; }

        uint32_t GetVersionID() const { return m_VersionID; }

        bool Map(uint32 subresource, const D3D12_RANGE* pReadRange, void** ppData, DataAccess access);
        bool Map(uint32 subresource, void** ppData, DataAccess access);

        void Unmap(uint32 subresource, const D3D12_RANGE* pWrittenRange);
        void Unmap(uint32 subresource);

        bool IsValid() const { return m_pResource != nullptr; }

        void GetHeapProperties(D3D12_HEAP_PROPERTIES& heapPropertiesOut, D3D12_HEAP_FLAGS& heapFlagsOut);

    protected:

        Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;
        D3D12_RESOURCE_STATES m_UsageState;
        D3D12_RESOURCE_STATES m_TransitioningState;
        D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;

        // Used to identify when a resource changes so descriptors can be copied etc.
        uint32_t m_VersionID = 0;

        bool m_Alive;
    };
}

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

#pragma once

#include "DX12/Final/DXGPUResource.h"

namespace RS::DX12
{
    //class EsramAllocator;

    class DXPixelBuffer : public DXGPUResource
    {
    public:
        DXPixelBuffer() : m_Width(0), m_Height(0), m_ArraySize(0), m_Format(DXGI_FORMAT_UNKNOWN), m_BankRotation(0) {}

        uint32_t GetWidth(void) const { return m_Width; }
        uint32_t GetHeight(void) const { return m_Height; }
        uint32_t GetDepth(void) const { return m_ArraySize; }
        const DXGI_FORMAT& GetFormat(void) const { return m_Format; }

        // Has no effect on Desktop
        void SetBankRotation(uint32_t RotationAmount)
        {
            (RotationAmount);
        }

        // Write the raw pixel buffer contents to a file
        // Note that data is preceded by a 16-byte header:  { DXGI_FORMAT, Pitch (in pixels), Width (in pixels), Height }
        void ExportToFile(const std::wstring& FilePath);

    protected:

        D3D12_RESOURCE_DESC DescribeTex2D(uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize, uint32_t NumMips, DXGI_FORMAT Format, UINT Flags);

        void AssociateWithResource(ID3D12Device* Device, const std::wstring& Name, ID3D12Resource* Resource, D3D12_RESOURCE_STATES CurrentState);

        void CreateTextureResource(ID3D12Device* Device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc,
            D3D12_CLEAR_VALUE ClearValue, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

        //void CreateTextureResource(ID3D12Device* Device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc,
        //    D3D12_CLEAR_VALUE ClearValue, EsramAllocator& Allocator);

        static DXGI_FORMAT GetBaseFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetUAVFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetDSVFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetDepthFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetStencilFormat(DXGI_FORMAT Format);
        static size_t BytesPerPixel(DXGI_FORMAT Format);

        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_ArraySize;
        DXGI_FORMAT m_Format;
        uint32_t m_BankRotation;
    };
}

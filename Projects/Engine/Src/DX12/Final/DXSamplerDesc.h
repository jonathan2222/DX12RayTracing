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

#pragma once

#include "DX12/Final/DX12Defines.h"
#include "Graphics/Color.h"

namespace RS::DX12
{
    class DXSamplerDesc : public D3D12_SAMPLER_DESC
    {
    public:
        // These defaults match the default values for HLSL-defined root
        // signature static samplers.  So not overriding them here means
        // you can safely not define them in HLSL.
        DXSamplerDesc()
        {
            Filter = D3D12_FILTER_ANISOTROPIC;
            AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            MipLODBias = 0.0f;
            MaxAnisotropy = 16;
            ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
            BorderColor[0] = 1.0f;
            BorderColor[1] = 1.0f;
            BorderColor[2] = 1.0f;
            BorderColor[3] = 1.0f;
            MinLOD = 0.0f;
            MaxLOD = D3D12_FLOAT32_MAX;
        }

        void SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE AddressMode)
        {
            AddressU = AddressMode;
            AddressV = AddressMode;
            AddressW = AddressMode;
        }

        void SetBorderColor(Color32 Border)
        {
            BorderColor[0] = Border.Red;
            BorderColor[1] = Border.Green;
            BorderColor[2] = Border.Blue;
            BorderColor[3] = Border.Alpha;
        }

        // Allocate new descriptor as needed; return handle to existing descriptor when possible
        D3D12_CPU_DESCRIPTOR_HANDLE CreateDescriptor(void);

        // Create descriptor in place (no deduplication).  Handle must be preallocated
        void CreateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE Handle);
    };
}

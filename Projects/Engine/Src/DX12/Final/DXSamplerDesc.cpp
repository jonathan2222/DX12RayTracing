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
// Author(s):  James Stanard
//             Alex Nankervis
//
// Modified by: Jonathan Åleskog

#include "PreCompiled.h"
#include "DXSamplerDesc.h"

#include "DX12/Final/DXCore.h"
#include "Utils/Misc/HashUtils.h"

#include <map>

namespace
{
    std::map< size_t, D3D12_CPU_DESCRIPTOR_HANDLE > s_SamplerCache;
}

D3D12_CPU_DESCRIPTOR_HANDLE RS::DX12::DXSamplerDesc::CreateDescriptor(void)
{
    size_t hashValue = Utils::Hash64State(this);
    auto iter = s_SamplerCache.find(hashValue);
    if (iter != s_SamplerCache.end())
    {
        return iter->second;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Handle = DXCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    DXCore::GetDevice()->CreateSampler(this, Handle);
    return Handle;
}

void RS::DX12::DXSamplerDesc::CreateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
    RS_ASSERT(Handle.ptr != 0 && Handle.ptr != -1);
    DXCore::GetDevice()->CreateSampler(this, Handle);
}

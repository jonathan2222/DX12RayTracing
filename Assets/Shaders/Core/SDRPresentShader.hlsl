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

#include "PresentRS.hlsli"

Texture2D<float3> MainBuffer : register(t0);
//Texture2D<float4> OverlayBuffer : register(t1);

float3 ApplySRGBCurve( float3 v )
{
    // Approximately pow(x, 1.0 / 2.2)
    return float3(
        v.x < 0.0031308 ? 12.92 * v.x : 1.055 * pow(v.x, 1.0 / 2.4) - 0.055,
        v.y < 0.0031308 ? 12.92 * v.y : 1.055 * pow(v.y, 1.0 / 2.4) - 0.055,
        v.z < 0.0031308 ? 12.92 * v.z : 1.055 * pow(v.z, 1.0 / 2.4) - 0.055
    );
}

float3 RemoveSRGBCurve( float3 v )
{
    // Approximately pow(x, 2.2)
    return float3(
        v.x < 0.04045 ? v.x / 12.92 : pow( (v.x + 0.055) / 1.055, 2.4 ),
        v.y < 0.04045 ? v.y / 12.92 : pow( (v.y + 0.055) / 1.055, 2.4 ),
        v.z < 0.04045 ? v.z / 12.92 : pow( (v.z + 0.055) / 1.055, 2.4 )
    );
}

#ifdef NEEDS_SCALING
    cbuffer Constants : register(b0)
    {
        float2 UVOffset;
    }

    SamplerState BilinearFilter : register(s0);

    float3 SampleColor(float2 uv)
    {
        return MainBuffer.SampleLevel(BilinearFilter, uv, 0);
    }

    float3 ScaleBuffer(float2 uv)
    {
        return 1.4 * SampleColor(uv) - 0.1 * (
            SampleColor(uv + float2(+UVOffset.x, +UVOffset.y)) +
            SampleColor(uv + float2(+UVOffset.x, -UVOffset.y)) +
            SampleColor(uv + float2(-UVOffset.x, +UVOffset.y)) +
            SampleColor(uv + float2(-UVOffset.x, -UVOffset.y))
            );
    }

    [RootSignature(Present_RootSig)]
    float3 PixelMain( float4 position : SV_Position, float2 uv : TexCoord0) : SV_Target0
    {
        float3 MainColor = ApplySRGBCurve(ScaleBuffer(uv));
        return MainColor;
    }

#else

    [RootSignature(Present_RootSig)]
    float3 PixelMain(float4 position : SV_Position) : SV_Target0
    {
        return ApplySRGBCurve(MainBuffer[(int2)position.xy]);
    }

#endif

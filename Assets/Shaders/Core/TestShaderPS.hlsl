
#include "CommonRootSignature.hlsli"

Texture2D<float3> ATexture : register(t0);
SamplerState LinearFilter : register(s0);

[RootSignature(Common_RootSig)]
void VertexMain(
    in uint VertID : SV_VertexID,
    out float4 Pos : SV_Position,
    out float2 Tex : TexCoord0
)
{
    // Texture coordinates range [0, 2], but only [0, 1] appears on screen.
    Tex = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
}

cbuffer Constants : register(b0)
{
    float scale;
}

[RootSignature(Common_RootSig)]
float3 PixelMain(float4 position : SV_Position, float2 uv : TexCoord0) : SV_Target0
{
    float3 color = ATexture.SampleLevel(LinearFilter, uv*scale, 0);
    return color;
}
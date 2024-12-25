#define NORMAL_SPACE space0
#define SAMPLER_SPACE space0
#define BINDLESS_TEXTURE_SPACE space2

struct PSInput
{
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float2 uv : UV0;
};

struct VSData
{
    float4x4 transform; // Object, World
    float4x4 camera; // View, Projection
};
ConstantBuffer<VSData> vsData : register(b2, NORMAL_SPACE);

struct InstanceData
{
    float4x4 transform;
};
StructuredBuffer<InstanceData> vsInstanceData : register(t2, BINDLESS_TEXTURE_SPACE);

PSInput VertexMain(float4 position : SV_POSITION, float4 normal : NORMAL, float2 uv : UV0, uint instanceIndex : SV_InstanceID)
{
    PSInput result;

    float4x4 transform = mul(vsData.transform, vsInstanceData[instanceIndex].transform);
    result.position = mul(mul(vsData.camera, transform), float4(position.xyz, 1.0f)); // Clip space.
    result.normal = mul(transform, float4(normal.xyz, 0.0f)); // World space
    result.uv = uv;

    return result;
}

struct PixelData
{
    float4 tint;
    uint4 texIndex;
};

struct PixelData2
{
    float4 tint;
};

ConstantBuffer<PixelData> viewData : register(b0, NORMAL_SPACE);
ConstantBuffer<PixelData2> viewData2 : register(b1, NORMAL_SPACE);

SamplerState pointSampler : register(s0, SAMPLER_SPACE);
//Texture2D diffseTex : register(t0, BINDLESS_TEXTURE_SPACE);
Texture2D diffseTex2D : register(t0, BINDLESS_TEXTURE_SPACE);
Texture2D nullTex2D : register(t1, BINDLESS_TEXTURE_SPACE);

float4 PixelMain(PSInput input) : SV_TARGET
{
    //float4 tex = diffseTex[viewData.texIndex.x].Sample(pointSampler, input.uv);
    //float4 nullTex = diffseTex[viewData.texIndex.y].Sample(pointSampler, input.uv);
    float4 tex = diffseTex2D.Sample(pointSampler, input.uv);
    float4 nullTex = nullTex2D.Sample(pointSampler, input.uv);
    float t = step(1.0, input.uv.x + input.uv.y);
    float4 finalTex = lerp(tex, nullTex, t);

    float3 lightDir = normalize(float3(0.5, -1.0, -0.2));
    float light = max(0, dot(input.normal.xyz, -lightDir));

    //float4 finalColor = float4(1.0f, 0.0f, 0.0f, 1.0f);
    return (finalTex * 0.1f + light * viewData.tint * viewData2.tint);
}
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;
};

PSInput VertexMain(float4 position : POSITION, float4 color : COLOR, float2 uv : UV)
{
    PSInput result;

    result.position = position;
    result.color = color;
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

#define NORMAL_SPACE space0
#define SAMPLER_SPACE space0
#define BINDLESS_TEXTURE_SPACE space2

ConstantBuffer<PixelData> viewData : register(b0, NORMAL_SPACE);
ConstantBuffer<PixelData2> viewData2 : register(b1, NORMAL_SPACE);

SamplerState pointSampler : register(s0, SAMPLER_SPACE);
Texture2D diffseTex[] : register(t0, BINDLESS_TEXTURE_SPACE);

float4 PixelMain(PSInput input) : SV_TARGET
{
    float4 tex = diffseTex[viewData.texIndex.x].Sample(pointSampler, input.uv);
    float4 nullTex = diffseTex[viewData.texIndex.y].Sample(pointSampler, input.uv);
    float t = step(1.0, input.uv.x + input.uv.y);
    float4 finalTex = lerp(tex, nullTex, t);
    return saturate(finalTex + input.color * viewData.tint * viewData2.tint);
}
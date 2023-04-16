struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VertexMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = position;
    result.color = color;

    return result;
}

struct PixelData
{
    float4 tint;
};

struct PixelData2
{
    float4 tint;
};

ConstantBuffer<PixelData> viewData : register(b0, space0);
ConstantBuffer<PixelData2> viewData2;

Texture2D diffseTex : register(t0, space0);

float4 PixelMain(PSInput input) : SV_TARGET
{
    return input.color * viewData.tint * viewData2.tint;
}
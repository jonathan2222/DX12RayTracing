struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : UV0;
};

struct Constants
{
    float4x4 projection;
    float3 textColor;
    float threshold;
};
ConstantBuffer<Constants> constants : register(b0, space0);

PSInput VertexMain(float4 position : SV_POSITION)
{
    PSInput result = (PSInput)0;
    result.position = mul(constants.projection, float4(position.xy, 0.f, 1.0f)); // Clip space.
    result.uv = position.zw;
    return result;
}

SamplerState texSampler : register(s0, space0);
Texture2D texture : register(t0, space0);

float4 PixelMain(PSInput input) : SV_TARGET
{
    float dist = texture.Sample(texSampler, input.uv).r;
    if (dist < constants.threshold)
        discard;
    return float4(constants.textColor, 1.f);
}
#define CBV_SPACE space0
#define SAMPLER_SPACE space0
#define SRV_SPACE space1

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : UV0;
};

struct EnvironmentData
{
    float4x4 camera; // View, Projection
};
ConstantBuffer<EnvironmentData> env : register(b0, CBV_SPACE);

PSInput VertexMain(float4 position : SV_POSITION, float2 uv : UV0, uint instanceIndex : SV_InstanceID)
{
    PSInput result = (PSInput)0;
 
    result.position = mul(env.camera, float4(position.xyz, 1.0f)); // Clip space.
    result.uv = uv;

    return result;
}

float4 PixelMain(PSInput input) : SV_TARGET
{
    float2 v = abs(input.uv-0.5);
    if (min(v.x, v.y) < 0.4)
        discard;
    return float4(0.8f, 0.8f, 0.8f, 1.f);
}
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
    float uvBorderWidth;
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
    float width = env.uvBorderWidth*0.5f;
    if (max(v.x, v.y) < max(0.f,0.5f-width))
        discard;
    return float4(0.8f, 0.8f, 0.8f, 1.f);
}
#define CBV_SPACE space0
#define SAMPLER_SPACE space0
#define SRV_SPACE space1

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : UV0;
    float3 color : CUSTOM1;
    uint type : CUSTOM0;
};

struct EnvironmentData
{
    float4x4 camera; // View, Projection
};
ConstantBuffer<EnvironmentData> env : register(b0, CBV_SPACE);

struct InstanceData
{
    float4x4 transform;
    float3 color;
    uint type;
};
StructuredBuffer<InstanceData> vsInstanceData : register(t0, SRV_SPACE);

PSInput VertexMain(float4 position : SV_POSITION, float2 uv : UV0, uint instanceIndex : SV_InstanceID)
{
    PSInput result = (PSInput)0;

    InstanceData instanceData = vsInstanceData[instanceIndex]; 
    result.position = mul(mul(env.camera, instanceData.transform), float4(position.xyz, 1.0f)); // Clip space.
    result.uv = uv;
    result.type = instanceData.type;
    result.color = instanceData.color;

    return result;
}

static uint TYPE_SQUARE = 0;
static uint TYPE_CIRCLE = 1;
static uint TYPE_BIT    = 2;

float4 PixelMain(PSInput input) : SV_TARGET
{
    float4 color = float4(input.color, 1.f);
    uint type = input.type;
    if (type == TYPE_SQUARE)
        return color;
    else if (type == TYPE_CIRCLE)
    {
        float2 v = (frac(input.uv)-float2(0.5,0.5)) * 2.f;
        if (dot(v,v) > 1.)
            discard;
        return color;
    }
    else if (type == TYPE_BIT)
    {
        return color;
    }
    return float4(1.f, 0.f, 1.f, 1.f);
}
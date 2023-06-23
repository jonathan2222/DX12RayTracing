
struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

PS_INPUT VertexMain(uint id : SV_VERTEXID)
{
    PS_INPUT output;
    output.uv = float2((id << 1) & 2, id & 2);
    output.pos  = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

SamplerState sampler0 : register(s0);
Texture2D texture0 : register(t0);

float4 PixelMain(PS_INPUT input) : SV_Target
{
    float4 out_col = texture0.Sample(sampler0, input.uv);
    return out_col;
}
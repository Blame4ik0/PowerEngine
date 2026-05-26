cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 Transform;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

struct VSInput
{
    float2 Position : POSITION;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 0.0f, 1.0f), Transform);
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
    return output;
}

float4 PS_Main(VSOutput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.TexCoord) * input.Color;
}
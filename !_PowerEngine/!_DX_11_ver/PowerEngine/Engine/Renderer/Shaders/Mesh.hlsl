cbuffer PerObjectBuffer : register(b0)
{
    row_major float4x4 WorldViewProjection;
};

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), WorldViewProjection);
    output.Normal = input.Normal;
    output.TexCoord = input.TexCoord;
    return output;
}

float4 PS_Main(VSOutput input) : SV_TARGET
{
    // Flat shading for now — just show normals as color
    // so we can verify geometry is correct before adding lighting
    float3 color = input.Normal * 0.5f + 0.5f;
    return float4(color, 1.0f);
}
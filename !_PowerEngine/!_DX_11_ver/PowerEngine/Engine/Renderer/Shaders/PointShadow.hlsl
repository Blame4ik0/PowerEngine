cbuffer PointShadowBuffer : register(b0)
{
    row_major float4x4 FaceMatrix;
    float3 LightPos;
    float LightRadius;
    row_major float4x4 World;
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
    float3 WorldPos : WORLDPOS;
};

VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    float4 worldPos = mul(float4(input.Position, 1.0f), World);
    output.WorldPos = worldPos.xyz;
    output.Position = mul(worldPos, FaceMatrix);
    return output;
}

float4 PS_Main(VSOutput input) : SV_TARGET
{
    // Store normalized linear distance — no hardware depth issues
    float dist = length(input.WorldPos - LightPos) / LightRadius;
    return float4(dist, dist, dist, 1.0f);
}
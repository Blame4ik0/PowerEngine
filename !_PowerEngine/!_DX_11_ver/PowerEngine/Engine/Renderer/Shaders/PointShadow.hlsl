cbuffer PointShadowPass : register(b0)
{
    row_major float4x4 FaceViewProj;
    row_major float4x4 World;
    float3 LightPos;
    float LightRadius;
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
    VSOutput o;
    float4 worldPos = mul(float4(input.Position, 1.0f), World);
    o.WorldPos = worldPos.xyz;
    o.Position = mul(worldPos, FaceViewProj);
    return o;
}

float4 PS_Main(VSOutput input) : SV_TARGET
{
    float dist = length(input.WorldPos - LightPos) / LightRadius;
    return float4(dist, dist, dist, 1.0f);
}

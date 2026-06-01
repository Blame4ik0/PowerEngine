cbuffer ShadowBuffer : register(b0)
{
    row_major float4x4 LightSpaceMatrix;
    row_major float4x4 World;
};

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

float4 VS_Main(VSInput input) : SV_POSITION
{
    float4 worldPos = mul(float4(input.Position, 1.0f), World);
    return mul(worldPos, LightSpaceMatrix);
}

void PS_Main()
{
}
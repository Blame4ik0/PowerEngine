cbuffer TransformBuffer : register(b0)
{
    float4x4 Transform;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

VSOutput VS_Main(
    float3 Position : POSITION,
    float4 Color : COLOR)
{
    VSOutput output;
    output.Position = mul(float4(Position, 1.0f), Transform);
    output.Color = Color;
    return output;
}

float4 PS_Main(VSOutput input) : SV_TARGET
{
    return input.Color;
}

// ---- Vertex Shader ----
float4 VS_Main(float2 Position : POSITION) : SV_POSITION
{
    return float4(Position, 0.0f, 1.0f);
}

// ---- Pixel Shader ----
float4 PS_Main() : SV_TARGET
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
cbuffer PerObjectBuffer : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 WorldViewProjection;
};

cbuffer PerFrameBuffer : register(b1)
{
    float3 CameraPosition;
    float _pad0;
};

cbuffer LightBuffer : register(b2)
{
    float3 DirLightDirection;
    float _pad1;
    float3 DirLightColor;
    float DirLightIntensity;

    float4 PointLightPositionRadius[4];
    float4 PointLightColorIntensity[4];

    int PointLightCount;
    float3 _pad2;
};

cbuffer MaterialBuffer : register(b3)
{
    float3 Albedo;
    float Metallic;
    float Roughness;
    float AmbientOcclusion;
    int UseAlbedoMap;
    int UseNormalMap;
    int UseSpecularMap;
    int UseGlossinessMap;
    float2 _pad3;
};

cbuffer ShadowBuffer : register(b4)
{
    row_major float4x4 LightSpaceMatrix;
    float ShadowBias;
    float3 _padS;
};

Texture2D g_albedoMap : register(t0);
Texture2D g_normalMap : register(t1);
Texture2D g_specularMap : register(t2);
Texture2D g_glossMap : register(t3);
Texture2D g_shadowMap : register(t5);

SamplerState g_sampler : register(s0);
SamplerComparisonState g_shadowSampler : register(s1);

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
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 LightSpacePos : LIGHTSPACEPOS;
};

VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    float4 worldPos = mul(float4(input.Position, 1.0f), World);
    output.WorldPos = worldPos.xyz;
    output.Position = mul(float4(input.Position, 1.0f), WorldViewProjection);
    output.Normal = normalize(mul(input.Normal, (float3x3) World));
    output.TexCoord = input.TexCoord;
    output.LightSpacePos = mul(worldPos, LightSpaceMatrix);
    return output;
}

static const float PI = 3.14159265359f;

float NDF_GGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * denom * denom);
}

float Geometry_SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

float Geometry_Smith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    return Geometry_SchlickGGX(NdotV, roughness) *
           Geometry_SchlickGGX(NdotL, roughness);
}

float3 Fresnel_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

float3 CookTorrance(float3 N, float3 V, float3 L,
                    float3 albedo, float metallic, float roughness,
                    float3 lightColor, float lightIntensity)
{
    float3 H = normalize(V + L);
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float D = NDF_GGX(N, H, roughness);
    float G = Geometry_Smith(N, V, L, roughness);
    float3 F = Fresnel_Schlick(max(dot(H, V), 0.0f), F0);
    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float3 specular = (D * G * F) / max(4.0f * NdotV * NdotL, 0.001f);
    float3 kD = (float3(1.0f, 1.0f, 1.0f) - F) * (1.0f - metallic);
    float3 diffuse = kD * albedo / PI;
    return (diffuse + specular) * lightColor * lightIntensity * NdotL;
}

float SampleShadowPCF(float4 lightSpacePos)
{
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    float2 uv;
    uv.x = projCoords.x * 0.5f + 0.5f;
    uv.y = -projCoords.y * 0.5f + 0.5f;

    if (uv.x < 0.0f || uv.x > 1.0f ||
        uv.y < 0.0f || uv.y > 1.0f ||
        projCoords.z > 1.0f)
        return 1.0f;

    float depth = projCoords.z - ShadowBias;
    float shadow = 0.0f;
    float texelSize = 1.0f / 2048.0f;

    [unroll]
    for (int x = -1; x <= 1; x++)
    {
        [unroll]
        for (int y = -1; y <= 1; y++)
        {
            float2 offset = float2(x, y) * texelSize;
            shadow += g_shadowMap.SampleCmpLevelZero(
                g_shadowSampler, uv + offset, depth);
        }
    }
    return shadow / 9.0f;
}

float4 PS_Main(VSOutput input) : SV_TARGET
{
    float3 N = normalize(input.Normal);
    float3 V = normalize(CameraPosition - input.WorldPos);

    float3 albedo = Albedo;
    float roughness = Roughness;
    float metallic = Metallic;

    if (UseAlbedoMap)
        albedo = g_albedoMap.Sample(g_sampler, input.TexCoord).rgb;
    if (UseGlossinessMap)
        roughness = 1.0f - g_glossMap.Sample(g_sampler, input.TexCoord).r;
    if (UseSpecularMap)
        metallic = g_specularMap.Sample(g_sampler, input.TexCoord).r;

    float shadowFactor = SampleShadowPCF(input.LightSpacePos);

    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    // Directional light — attenuated by shadow
    float3 L = normalize(-DirLightDirection);
    Lo += CookTorrance(N, V, L, albedo, metallic, roughness,
                       DirLightColor, DirLightIntensity) * shadowFactor;

    // Point lights — no shadow
    for (int i = 0; i < PointLightCount; i++)
    {
        float3 lightPos = PointLightPositionRadius[i].xyz;
        float lightRadius = PointLightPositionRadius[i].w;
        float3 lightColor = PointLightColorIntensity[i].xyz;
        float lightIntens = PointLightColorIntensity[i].w;

        float3 toLight = lightPos - input.WorldPos;
        float distance = length(toLight);
        float3 Lp = normalize(toLight);
        float attenuation = 1.0f / (distance * distance);
        float radiusFactor = saturate(1.0f - (distance / lightRadius));
        attenuation *= radiusFactor * radiusFactor;

        Lo += CookTorrance(N, V, Lp, albedo, metallic, roughness,
                           lightColor, lightIntens * attenuation);
    }

    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * AmbientOcclusion;
    float3 color = ambient + Lo;
    color = color / (color + float3(1.0f, 1.0f, 1.0f));
    color = pow(color, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

    return float4(color, 1.0f);
}

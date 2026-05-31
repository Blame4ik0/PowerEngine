// ============================================================
//  Mesh.hlsl — PBR shading with multiple lights
// ============================================================

// ---- Constant buffers ----

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
    // Directional light
    float3 DirLightDirection;
    float _pad1;
    float3 DirLightColor;
    float DirLightIntensity;

    // Point lights (max 4)
    float3 PointLightPosition[4];
    float PointLightRadius[4];
    float3 PointLightColor[4];
    float PointLightIntensity[4];
    int PointLightCount;
    float3 _pad2;
};

cbuffer MaterialBuffer : register(b3)
{
    float3 Albedo;
    float Metallic;
    float Roughness;
    float AmbientOcclusion;
    float2 _pad3;
};

// ---- Vertex IO ----

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
};

// ---- Vertex Shader ----

VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    float4 worldPos = mul(float4(input.Position, 1.0f), World);
    output.WorldPos = worldPos.xyz;
    output.Position = mul(float4(input.Position, 1.0f), WorldViewProjection);
    output.Normal = normalize(mul(input.Normal, (float3x3) World));
    output.TexCoord = input.TexCoord;
    return output;
}

// ---- PBR Functions ----

static const float PI = 3.14159265359f;

// Normal Distribution Function — GGX/Trowbridge-Reitz
// Describes how microfacets are distributed on the surface
float NDF_GGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * denom * denom);
}

// Geometry function — Schlick-GGX
// Models self-shadowing of microfacets
float Geometry_SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

// Smith's method — combines view and light geometry terms
float Geometry_Smith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    return Geometry_SchlickGGX(NdotV, roughness) *
           Geometry_SchlickGGX(NdotL, roughness);
}

// Fresnel — Schlick approximation
// How reflectivity changes with viewing angle
float3 Fresnel_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

// ---- Cook-Torrance BRDF for one light ----
float3 CookTorrance(float3 N, float3 V, float3 L,
                    float3 albedo, float metallic, float roughness,
                    float3 lightColor, float lightIntensity)
{
    float3 H = normalize(V + L);

    // Base reflectivity — non-metals reflect 4%, metals use albedo color
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float D = NDF_GGX(N, H, roughness);
    float G = Geometry_Smith(N, V, L, roughness);
    float3 F = Fresnel_Schlick(max(dot(H, V), 0.0f), F0);

    // Specular term
    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float3 specular = (D * G * F) /
                      max(4.0f * NdotV * NdotL, 0.001f);

    // Diffuse — metals have no diffuse (all energy goes to specular)
    float3 kD = (float3(1.0f, 1.0f, 1.0f) - F) * (1.0f - metallic);
    float3 diffuse = kD * albedo / PI;

    return (diffuse + specular) * lightColor * lightIntensity * NdotL;
}

// ---- Pixel Shader ----

float4 PS_Main(VSOutput input) : SV_TARGET
{
    float3 N = normalize(input.Normal);
    float3 V = normalize(CameraPosition - input.WorldPos);

    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    // Directional light
    float3 L = normalize(-DirLightDirection);
    Lo += CookTorrance(N, V, L,
                       Albedo, Metallic, Roughness,
                       DirLightColor, DirLightIntensity);

    // Point lights
    for (int i = 0; i < PointLightCount; i++)
    {
        float3 toLight = PointLightPosition[i] - input.WorldPos;
        float distance = length(toLight);
        float3 Lp = normalize(toLight);

        // Attenuation — physically correct inverse square falloff
        float attenuation = 1.0f / (distance * distance);

        // Soft radius cutoff
        float radiusFactor = saturate(1.0f - (distance / PointLightRadius[i]));
        attenuation *= radiusFactor * radiusFactor;

        Lo += CookTorrance(N, V, Lp,
                           Albedo, Metallic, Roughness,
                           PointLightColor[i],
                           PointLightIntensity[i] * attenuation);
    }

    // Ambient — simple IBL approximation
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * Albedo * AmbientOcclusion;

    float3 color = ambient + Lo;

    // Tone mapping — Reinhard operator
    // Maps HDR values back to 0-1 range
    color = color / (color + float3(1.0f, 1.0f, 1.0f));

    // Gamma correction — convert linear to sRGB
    color = pow(color, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

    return float4(color, 1.0f);
}
#pragma once
#include <DirectXMath.h>
#include <string>
#include <memory>

namespace Engine
{
    class Texture2D;

    struct DirectionalLight
    {
        DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.5f };
        float             _pad0 = 0.0f;
        DirectX::XMFLOAT3 Color = { 1.0f,  1.0f, 1.0f };
        float             Intensity = 1.0f;
    };

    struct PointLight
    {
        DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
        float             Radius = 10.0f;
        DirectX::XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f };
        float             Intensity = 10.0f;
    };

    struct Material
    {
        // PBR base values — used when no texture is bound
        DirectX::XMFLOAT3 Albedo = { 1.0f, 1.0f, 1.0f };
        float             Metallic = 0.0f;
        float             Roughness = 0.5f;
        float             AmbientOcclusion = 1.0f;

        // Option A: file paths — loaded and cached on first use
        std::string AlbedoMap;
        std::string NormalMap;
        std::string SpecularMap;
        std::string GlossinessMap;

        // Option B: pre-loaded textures — take priority over paths
        std::shared_ptr<Texture2D> AlbedoTex;
        std::shared_ptr<Texture2D> NormalTex;
        std::shared_ptr<Texture2D> SpecularTex;
        std::shared_ptr<Texture2D> GlossinessTex;
    };
}
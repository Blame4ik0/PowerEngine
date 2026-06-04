#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <string>

namespace Engine
{
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
        DirectX::XMFLOAT3 Albedo = { 1.0f, 1.0f, 1.0f };
        float Metallic = 0.0f;
        float Roughness = 0.5f;
        float AmbientOcclusion = 1.0f;

        // Manual texture assignment (highest priority)
        std::string AlbedoMap;
        std::string NormalMap;
        std::string SpecularMap;
        std::string GlossinessMap;

        // For future: embedded texture support
        std::shared_ptr<Texture2D> AlbedoTexture = nullptr;
        std::shared_ptr<Texture2D> NormalTexture = nullptr;
    };
}
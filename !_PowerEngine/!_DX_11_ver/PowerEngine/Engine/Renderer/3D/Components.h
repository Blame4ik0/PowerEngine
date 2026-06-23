#pragma once
#include <DirectXMath.h>
#include <memory>
#include <string>
#include "Renderer/3D/Mesh.h"
#include "Renderer/3D/Light.h"

namespace Engine
{
    // ---- Transform ----
    struct TransformComponent
    {
        DirectX::XMFLOAT3 Position = { 0, 0, 0 };
        DirectX::XMFLOAT3 Rotation = { 0, 0, 0 }; // degrees
        DirectX::XMFLOAT3 Scale = { 1, 1, 1 };

        DirectX::XMMATRIX GetWorldMatrix() const
        {
            using namespace DirectX;
            return XMMatrixScaling(Scale.x, Scale.y, Scale.z) *
                XMMatrixRotationRollPitchYaw(
                    XMConvertToRadians(Rotation.x),
                    XMConvertToRadians(Rotation.y),
                    XMConvertToRadians(Rotation.z)) *
                XMMatrixTranslation(Position.x, Position.y, Position.z);
        }
    };

    // ---- Mesh / rendering ----
    struct MeshComponent
    {
        std::shared_ptr<Engine::Mesh> Mesh;

        // true  -> render via DrawMeshAuto (uses embedded materials, e.g. GLTF/FBX)
        // false -> render via DrawMesh with the entity's MaterialComponent
        bool UseAutoMaterial = false;
    };

    struct MaterialComponent
    {
        Engine::Material Material;
    };

    // Optional — only attach if you want to override embedded materials
    // on a DrawMeshAuto entity (e.g. force the sword blade metallic).
    struct MaterialOverrideComponent
    {
        Engine::MaterialOverride Override;
        int MaterialIndex = -1; // -1 = applies globally to all submeshes
    };

    // ---- Lights ----
    struct DirectionalLightComponent
    {
        Engine::DirectionalLight Light;
    };

    struct PointLightComponent
    {
        Engine::PointLight Light;
    };

    // ---- Editor / misc ----
    struct NameComponent
    {
        std::string Name;
    };

    // Marks an entity as not yet uploaded to GPU / not ready to draw
    struct PendingLoadComponent
    {
        std::string Filepath;
        UVMode      UvMode = UVMode::FlipV;
    };
}
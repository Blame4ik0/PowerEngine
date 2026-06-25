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

    // Describes HOW a mesh was created, so SceneSerializer can rebuild it
    // on load without ever serializing raw vertex/index data.
    enum class MeshSourceType { File, Plane, Cube, Sphere };

    struct MeshSourceComponent
    {
        MeshSourceType Type = MeshSourceType::File;

        // Used when Type == File
        std::string Filepath;
        UVMode      UvMode = UVMode::FlipV;

        // Used when Type == Plane
        float Width = 1.0f;
        float Height = 1.0f;

        // Used when Type == Cube
        float Size = 1.0f;

        // Used when Type == Sphere
        float Radius = 1.0f;
        int   Slices = 16;
        int   Stacks = 16;
    };

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

    // Optional — attach to a point light entity to make its color
    // cycle through the hue wheel over time. BaseHue survives save/load
    // so the animation phase doesn't depend on registry iteration order.
    struct RGBCyclerComponent
    {
        float BaseHue = 0.0f;
        float DegreesPerSecond = 40.0f;
    };

    // ---- Editor / misc ----
    struct NameComponent
    {
        std::string Name;
    };
}
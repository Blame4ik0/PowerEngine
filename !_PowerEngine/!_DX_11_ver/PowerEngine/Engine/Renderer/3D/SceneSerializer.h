#pragma once
#include "Scene.h"
#include <string>

namespace Engine
{
    // Saves/loads a Scene's entt::registry to/from a JSON file.
    // Mesh geometry is never serialized directly — only the
    // MeshSourceComponent (file path or primitive params) is saved,
    // and the actual GPU mesh is rebuilt on load via device/ctx.
    class SceneSerializer
    {
    public:
        static bool Save(const Scene& scene, const std::string& filepath);

        // device/ctx are required to rebuild meshes and load textures
        // referenced by MaterialComponent / MeshSourceComponent.
        static bool Load(Scene& scene, const std::string& filepath,
            ID3D11Device* device, ID3D11DeviceContext* ctx);
    };
}
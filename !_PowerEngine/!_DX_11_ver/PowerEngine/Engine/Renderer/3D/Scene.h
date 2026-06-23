#pragma once
#include <entt/entt.hpp>
#include "Components.h"
#include "Renderer/3D/Renderer3D.h"

namespace Engine
{
    // Thin wrapper around entt::registry. Owns all entities/components
    // for one scene and knows how to draw itself via Renderer3D.
    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        entt::entity CreateEntity(const std::string& name = "Entity")
        {
            auto e = m_registry.create();
            m_registry.emplace<NameComponent>(e, name);
            m_registry.emplace<TransformComponent>(e);
            return e;
        }

        void DestroyEntity(entt::entity e)
        {
            m_registry.destroy(e);
        }

        entt::registry& Registry() { return m_registry; }
        const entt::registry& Registry() const { return m_registry; }

        // ---- Rendering ----
        // Call once per frame inside whichever pass is active
        // (shadow / point-shadow / main) — Renderer3D's BeginShadowPass /
        // BeginPointShadowPass / BeginScene determine the actual pass.
        void Draw(Renderer3D& renderer3D)
        {
            auto view = m_registry.view<TransformComponent, MeshComponent>();
            for (auto entity : view)
            {
                auto& transform = view.get<TransformComponent>(entity);
                auto& meshComp = view.get<MeshComponent>(entity);

                if (!meshComp.Mesh || !meshComp.Mesh->IsLoaded())
                    continue;

                DirectX::XMMATRIX world = transform.GetWorldMatrix();

                // Apply material override, if present, before drawing
                if (auto* ov = m_registry.try_get<MaterialOverrideComponent>(entity))
                {
                    if (ov->MaterialIndex < 0)
                        meshComp.Mesh->SetMaterialOverride(ov->Override);
                    else
                        meshComp.Mesh->SetMaterialOverride(ov->MaterialIndex, ov->Override);
                }

                if (meshComp.UseAutoMaterial)
                {
                    renderer3D.DrawMeshAuto(*meshComp.Mesh, world);
                }
                else
                {
                    const Material* mat = nullptr;
                    if (auto* matComp = m_registry.try_get<MaterialComponent>(entity))
                        mat = &matComp->Material;

                    renderer3D.DrawMesh(*meshComp.Mesh, world,
                        mat ? *mat : Material{});
                }
            }
        }

        // Pushes all lights in the scene into Renderer3D.
        // Call once per frame before BeginScene/BeginShadowPass.
        void UpdateLights(Renderer3D& renderer3D)
        {
            renderer3D.ClearPointLights();

            auto dirView = m_registry.view<DirectionalLightComponent>();
            for (auto entity : dirView)
            {
                renderer3D.SetDirectionalLight(
                    dirView.get<DirectionalLightComponent>(entity).Light);
                break; // only one directional light supported currently
            }

            auto ptView = m_registry.view<PointLightComponent>();
            for (auto entity : ptView)
            {
                renderer3D.AddPointLight(
                    ptView.get<PointLightComponent>(entity).Light);
            }
        }

    private:
        entt::registry m_registry;
    };
}
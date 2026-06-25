#include "SceneSerializer.h"
#include "Core/Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;
using namespace DirectX;

namespace Engine
{
    // ---- Small helpers: XMFLOAT3 <-> JSON array ----

    static json ToJson(const XMFLOAT3& v)
    {
        return json::array({ v.x, v.y, v.z });
    }

    static XMFLOAT3 Float3FromJson(const json& j)
    {
        return { j[0].get<float>(), j[1].get<float>(), j[2].get<float>() };
    }

    // ---- Component serializers ----

    static json Serialize(const TransformComponent& t)
    {
        return {
            { "position", ToJson(t.Position) },
            { "rotation", ToJson(t.Rotation) },
            { "scale",    ToJson(t.Scale) }
        };
    }

    static TransformComponent DeserializeTransform(const json& j)
    {
        TransformComponent t;
        t.Position = Float3FromJson(j["position"]);
        t.Rotation = Float3FromJson(j["rotation"]);
        t.Scale = Float3FromJson(j["scale"]);
        return t;
    }

    static const char* MeshSourceTypeToStr(MeshSourceType t)
    {
        switch (t)
        {
        case MeshSourceType::File:   return "file";
        case MeshSourceType::Plane:  return "plane";
        case MeshSourceType::Cube:   return "cube";
        case MeshSourceType::Sphere: return "sphere";
        }
        return "file";
    }

    static MeshSourceType MeshSourceTypeFromStr(const std::string& s)
    {
        if (s == "plane")  return MeshSourceType::Plane;
        if (s == "cube")   return MeshSourceType::Cube;
        if (s == "sphere") return MeshSourceType::Sphere;
        return MeshSourceType::File;
    }

    static json Serialize(const MeshSourceComponent& m)
    {
        json j;
        j["type"] = MeshSourceTypeToStr(m.Type);
        switch (m.Type)
        {
        case MeshSourceType::File:
            j["filepath"] = m.Filepath;
            j["uvMode"] = static_cast<int>(m.UvMode);
            break;
        case MeshSourceType::Plane:
            j["width"] = m.Width;
            j["height"] = m.Height;
            break;
        case MeshSourceType::Cube:
            j["size"] = m.Size;
            break;
        case MeshSourceType::Sphere:
            j["radius"] = m.Radius;
            j["slices"] = m.Slices;
            j["stacks"] = m.Stacks;
            break;
        }
        return j;
    }

    static MeshSourceComponent DeserializeMeshSource(const json& j)
    {
        MeshSourceComponent m;
        m.Type = MeshSourceTypeFromStr(j["type"].get<std::string>());
        switch (m.Type)
        {
        case MeshSourceType::File:
            m.Filepath = j["filepath"].get<std::string>();
            m.UvMode = static_cast<UVMode>(j.value("uvMode", 1));
            break;
        case MeshSourceType::Plane:
            m.Width = j.value("width", 1.0f);
            m.Height = j.value("height", 1.0f);
            break;
        case MeshSourceType::Cube:
            m.Size = j.value("size", 1.0f);
            break;
        case MeshSourceType::Sphere:
            m.Radius = j.value("radius", 1.0f);
            m.Slices = j.value("slices", 16);
            m.Stacks = j.value("stacks", 16);
            break;
        }
        return m;
    }

    static json Serialize(const MaterialComponent& mc)
    {
        const Material& m = mc.Material;
        return {
            { "albedo",         ToJson(m.Albedo) },
            { "metallic",       m.Metallic },
            { "roughness",      m.Roughness },
            { "ambientOcc",     m.AmbientOcclusion },
            { "albedoMap",      m.AlbedoMap },
            { "normalMap",      m.NormalMap },
            { "specularMap",    m.SpecularMap },
            { "glossinessMap",  m.GlossinessMap }
        };
    }

    static MaterialComponent DeserializeMaterial(const json& j)
    {
        MaterialComponent mc;
        mc.Material.Albedo = Float3FromJson(j["albedo"]);
        mc.Material.Metallic = j.value("metallic", 0.0f);
        mc.Material.Roughness = j.value("roughness", 0.5f);
        mc.Material.AmbientOcclusion = j.value("ambientOcc", 1.0f);
        mc.Material.AlbedoMap = j.value("albedoMap", "");
        mc.Material.NormalMap = j.value("normalMap", "");
        mc.Material.SpecularMap = j.value("specularMap", "");
        mc.Material.GlossinessMap = j.value("glossinessMap", "");
        return mc;
    }

    static json Serialize(const MaterialOverrideComponent& oc)
    {
        const MaterialOverride& o = oc.Override;
        return {
            { "materialIndex",     oc.MaterialIndex },
            { "overrideAlbedo",    o.overrideAlbedo },
            { "albedo",            ToJson(o.albedo) },
            { "overrideMetallic",  o.overrideMetallic },
            { "metallic",          o.metallic },
            { "overrideRoughness", o.overrideRoughness },
            { "roughness",         o.roughness }
        };
    }

    static MaterialOverrideComponent DeserializeMaterialOverride(const json& j)
    {
        MaterialOverrideComponent oc;
        oc.MaterialIndex = j.value("materialIndex", -1);
        oc.Override.overrideAlbedo = j.value("overrideAlbedo", false);
        oc.Override.albedo = Float3FromJson(j["albedo"]);
        oc.Override.overrideMetallic = j.value("overrideMetallic", false);
        oc.Override.metallic = j.value("metallic", 0.0f);
        oc.Override.overrideRoughness = j.value("overrideRoughness", false);
        oc.Override.roughness = j.value("roughness", 0.5f);
        return oc;
    }

    static json Serialize(const DirectionalLightComponent& lc)
    {
        const DirectionalLight& l = lc.Light;
        return {
            { "direction", ToJson(l.Direction) },
            { "color",     ToJson(l.Color) },
            { "intensity", l.Intensity }
        };
    }

    static DirectionalLightComponent DeserializeDirLight(const json& j)
    {
        DirectionalLightComponent lc;
        lc.Light.Direction = Float3FromJson(j["direction"]);
        lc.Light.Color = Float3FromJson(j["color"]);
        lc.Light.Intensity = j.value("intensity", 1.0f);
        return lc;
    }

    static json Serialize(const PointLightComponent& lc)
    {
        const PointLight& l = lc.Light;
        return {
            { "position",  ToJson(l.Position) },
            { "color",     ToJson(l.Color) },
            { "intensity", l.Intensity },
            { "radius",    l.Radius }
        };
    }

    static PointLightComponent DeserializePointLight(const json& j)
    {
        PointLightComponent lc;
        lc.Light.Position = Float3FromJson(j["position"]);
        lc.Light.Color = Float3FromJson(j["color"]);
        lc.Light.Intensity = j.value("intensity", 1.0f);
        lc.Light.Radius = j.value("radius", 10.0f);
        return lc;
    }

    static json Serialize(const RGBCyclerComponent& rc)
    {
        return {
            { "baseHue",          rc.BaseHue },
            { "degreesPerSecond", rc.DegreesPerSecond }
        };
    }

    static RGBCyclerComponent DeserializeRGBCycler(const json& j)
    {
        RGBCyclerComponent rc;
        rc.BaseHue = j.value("baseHue", 0.0f);
        rc.DegreesPerSecond = j.value("degreesPerSecond", 40.0f);
        return rc;
    }

    // ---- Mesh rebuild from MeshSourceComponent ----

    static std::shared_ptr<Mesh> RebuildMesh(
        const MeshSourceComponent& src,
        ID3D11Device* device, ID3D11DeviceContext* ctx)
    {
        auto mesh = std::make_shared<Mesh>();
        bool ok = false;

        switch (src.Type)
        {
        case MeshSourceType::File:
            ok = mesh->Load(device, ctx, src.Filepath, src.UvMode);
            break;
        case MeshSourceType::Plane:
            ok = mesh->CreatePlane(device, src.Width, src.Height);
            break;
        case MeshSourceType::Cube:
            ok = mesh->CreateCube(device, src.Size);
            break;
        case MeshSourceType::Sphere:
            ok = mesh->CreateSphere(device, src.Radius, src.Slices, src.Stacks);
            break;
        }

        if (!ok)
        {
            LOG_ERROR("SceneSerializer: failed to rebuild mesh (type={}).",
                MeshSourceTypeToStr(src.Type));
            return nullptr;
        }
        return mesh;
    }

    // ================================================================
    //  Save
    // ================================================================

    bool SceneSerializer::Save(const Scene& scene, const std::string& filepath)
    {
        json root;
        root["version"] = 1;
        json entitiesJson = json::array();

        auto& registry = scene.Registry();

        // Iterate every entity that has at least a TransformComponent
        // (every entity created via Scene::CreateEntity has one).
        auto view = registry.view<const TransformComponent>();
        for (auto entity : view)
        {
            json ej;
            ej["id"] = static_cast<uint32_t>(entity);

            if (auto* n = registry.try_get<NameComponent>(entity))
                ej["name"] = n->Name;

            ej["transform"] = Serialize(view.get<const TransformComponent>(entity));

            if (auto* ms = registry.try_get<MeshSourceComponent>(entity))
                ej["meshSource"] = Serialize(*ms);

            if (auto* mc = registry.try_get<MeshComponent>(entity))
                ej["useAutoMaterial"] = mc->UseAutoMaterial;

            if (auto* mat = registry.try_get<MaterialComponent>(entity))
                ej["material"] = Serialize(*mat);

            if (auto* ov = registry.try_get<MaterialOverrideComponent>(entity))
                ej["materialOverride"] = Serialize(*ov);

            if (auto* dl = registry.try_get<DirectionalLightComponent>(entity))
                ej["directionalLight"] = Serialize(*dl);

            if (auto* pl = registry.try_get<PointLightComponent>(entity))
                ej["pointLight"] = Serialize(*pl);

            if (auto* rc = registry.try_get<RGBCyclerComponent>(entity))
                ej["rgbCycler"] = Serialize(*rc);

            entitiesJson.push_back(ej);
        }

        root["entities"] = entitiesJson;

        std::ofstream file(filepath);
        if (!file.is_open())
        {
            LOG_ERROR("SceneSerializer: could not open '{}' for writing.", filepath);
            return false;
        }
        file << root.dump(2);

        LOG_INFO("SceneSerializer: saved {} entities to '{}'.",
            entitiesJson.size(), filepath);
        return true;
    }

    // ================================================================
    //  Load
    // ================================================================

    bool SceneSerializer::Load(Scene& scene, const std::string& filepath,
        ID3D11Device* device, ID3D11DeviceContext* ctx)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            LOG_ERROR("SceneSerializer: could not open '{}' for reading.", filepath);
            return false;
        }

        json root;
        try
        {
            file >> root;
        }
        catch (const json::parse_error& e)
        {
            LOG_ERROR("SceneSerializer: JSON parse error in '{}': {}",
                filepath, e.what());
            return false;
        }

        // Clear existing scene before loading
        scene.Registry().clear();

        auto& registry = scene.Registry();
        int loadedCount = 0;

        for (auto& ej : root["entities"])
        {
            std::string name = ej.value("name", "Entity");
            auto entity = scene.CreateEntity(name); // adds Transform + Name

            if (ej.contains("transform"))
                registry.replace<TransformComponent>(entity,
                    DeserializeTransform(ej["transform"]));

            std::shared_ptr<Mesh> mesh;
            if (ej.contains("meshSource"))
            {
                MeshSourceComponent src = DeserializeMeshSource(ej["meshSource"]);
                mesh = RebuildMesh(src, device, ctx);
                registry.emplace<MeshSourceComponent>(entity, src);
            }

            if (mesh)
            {
                bool useAuto = ej.value("useAutoMaterial", false);
                registry.emplace<MeshComponent>(entity,
                    MeshComponent{ mesh, useAuto });
            }

            if (ej.contains("material"))
                registry.emplace<MaterialComponent>(entity,
                    DeserializeMaterial(ej["material"]));

            if (ej.contains("materialOverride"))
                registry.emplace<MaterialOverrideComponent>(entity,
                    DeserializeMaterialOverride(ej["materialOverride"]));

            if (ej.contains("directionalLight"))
                registry.emplace<DirectionalLightComponent>(entity,
                    DeserializeDirLight(ej["directionalLight"]));

            if (ej.contains("pointLight"))
                registry.emplace<PointLightComponent>(entity,
                    DeserializePointLight(ej["pointLight"]));

            if (ej.contains("rgbCycler"))
                registry.emplace<RGBCyclerComponent>(entity,
                    DeserializeRGBCycler(ej["rgbCycler"]));

            loadedCount++;
        }

        LOG_INFO("SceneSerializer: loaded {} entities from '{}'.",
            loadedCount, filepath);
        return true;
    }
}
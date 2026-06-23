#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace Engine
{
    using Microsoft::WRL::ComPtr;

    class Texture2D;

    struct Vertex3D
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT2 TexCoord;
        DirectX::XMFLOAT3 Tangent;
    };

    enum class UVMode { Default, FlipV, FlipU, FlipBoth };

    struct EmbeddedTexture
    {
        std::string                name;
        std::shared_ptr<Texture2D> texture;
    };

    struct SubMesh
    {
        ComPtr<ID3D11Buffer> vertexBuffer;
        ComPtr<ID3D11Buffer> indexBuffer;
        int                  indexCount = 0;
        int                  materialIndex = -1;
    };

    struct MeshMaterial
    {
        std::string name;
        std::shared_ptr<Texture2D> albedo;
        std::shared_ptr<Texture2D> normal;
        std::shared_ptr<Texture2D> metallicRoughness;
        std::shared_ptr<Texture2D> specular;
        std::shared_ptr<Texture2D> glossiness;
        DirectX::XMFLOAT3 albedoFactor = { 1, 1, 1 };
        float             metallicFactor = 0.0f;
        float             roughnessFactor = 0.5f;
        bool isMetallicRoughness = true;
    };

    struct MaterialOverride
    {
        bool               overrideAlbedo = false;
        DirectX::XMFLOAT3  albedo = { 1, 1, 1 };
        bool               overrideMetallic = false;
        float              metallic = 0.0f;
        bool               overrideRoughness = false;
        float              roughness = 0.5f;
    };

    // Axis-aligned bounding box in local space
    struct AABB
    {
        DirectX::XMFLOAT3 min = { 1e9f,  1e9f,  1e9f };
        DirectX::XMFLOAT3 max = { -1e9f, -1e9f, -1e9f };

        // Returns the 8 corners transformed by a world matrix
        void GetCorners(const DirectX::XMMATRIX& world,
            DirectX::XMFLOAT3 out[8]) const;
    };

    class Mesh
    {
    public:
        Mesh() = default;
        ~Mesh() = default;

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        // ---- Loading ----
        bool Load(ID3D11Device* device, ID3D11DeviceContext* ctx,
            const std::string& filepath,
            UVMode uvMode = UVMode::FlipV);
        bool CreateCube(ID3D11Device* device, float size = 1.0f);
        bool CreatePlane(ID3D11Device* device, float width = 1.0f,
            float height = 1.0f);
        bool CreateSphere(ID3D11Device* device, float radius = 1.0f,
            int slices = 16, int stacks = 16);

        // ---- Transform ----
        void SetPosition(float x, float y, float z);
        void SetRotation(float degX, float degY, float degZ);
        void SetScale(float x, float y, float z);
        void SetScale(float uniform);
        void Move(float dx, float dy, float dz);
        void Rotate(float degX, float degY, float degZ);

        DirectX::XMFLOAT3 GetPosition() const { return m_position; }
        DirectX::XMFLOAT3 GetRotation() const { return m_rotation; }
        DirectX::XMFLOAT3 GetScale()    const { return m_scale; }
        DirectX::XMMATRIX GetWorldMatrix() const;

        // ---- Draw ----
        void Draw(ID3D11DeviceContext* ctx) const;
        void DrawSubMesh(ID3D11DeviceContext* ctx, int subMeshIndex) const;

        // Used by DrawMeshInstanced — binds VB to slot 0, instance buffer to slot 1
        void BindBuffers(ID3D11DeviceContext* ctx,
            ID3D11Buffer* instanceBuffer,
            int instanceCount) const;

        // ---- Data access ----
        bool IsLoaded()         const { return m_loaded; }
        int  GetIndexCount()    const { return m_totalIndexCount; }
        int  GetSubMeshCount()  const { return (int)m_subMeshes.size(); }
        int  GetMaterialCount() const { return (int)m_materials.size(); }

        const SubMesh& GetSubMesh(int i) const { return m_subMeshes[i]; }
        const MeshMaterial& GetMaterial(int i) const { return m_materials[i]; }
        const AABB& GetAABB()      const { return m_aabb; }

        const std::vector<EmbeddedTexture>& GetEmbeddedTextures() const
        {
            return m_embeddedTextures;
        }

        bool HasEmbeddedMaterials() const { return !m_materials.empty(); }

        // ---- Material overrides ----
        void SetMaterialOverride(const MaterialOverride & override);
        void SetMaterialOverride(int materialIndex, const MaterialOverride & override);
        void ClearMaterialOverrides();
        const MaterialOverride* ResolveOverride(int materialIndex) const;

    private:
        void ComputeAABB(const std::vector<Vertex3D>& vertices);
        void ExpandAABB(const std::vector<Vertex3D>& vertices);

        bool UploadSubMesh(ID3D11Device* device,
            const std::vector<Vertex3D>& vertices,
            const std::vector<uint32_t>& indices,
            int materialIndex, SubMesh& out);

        bool UploadSingle(ID3D11Device* device,
            const std::vector<Vertex3D>& vertices,
            const std::vector<uint32_t>& indices);

        std::vector<SubMesh>         m_subMeshes;
        std::vector<MeshMaterial>    m_materials;
        std::vector<EmbeddedTexture> m_embeddedTextures;

        bool             m_hasGlobalOverride = false;
        MaterialOverride m_globalOverride;
        std::unordered_map<int, MaterialOverride> m_materialOverrides;

        ComPtr<ID3D11Buffer> m_vertexBuffer;
        ComPtr<ID3D11Buffer> m_indexBuffer;

        AABB              m_aabb;
        int               m_totalIndexCount = 0;
        bool              m_loaded = false;
        bool              m_hasSubMeshes = false;

        DirectX::XMFLOAT3 m_position = { 0, 0, 0 };
        DirectX::XMFLOAT3 m_rotation = { 0, 0, 0 };
        DirectX::XMFLOAT3 m_scale = { 1, 1, 1 };

        ID3D11Device* m_device = nullptr;
    };
}
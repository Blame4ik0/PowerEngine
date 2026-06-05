#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <memory>

namespace Engine
{
    using Microsoft::WRL::ComPtr;

    class Texture2D;

    struct Vertex3D
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT2 TexCoord;
    };

    enum class UVMode { Default, FlipV, FlipU, FlipBoth };

    struct EmbeddedTexture
    {
        std::string                name;
        std::shared_ptr<Texture2D> texture;
    };

    // A single draw call with its own material index
    struct SubMesh
    {
        ComPtr<ID3D11Buffer> vertexBuffer;
        ComPtr<ID3D11Buffer> indexBuffer;
        int                  indexCount = 0;
        int                  materialIndex = -1; // index into m_materials
    };

    // Material data extracted from the model file
    struct MeshMaterial
    {
        std::string name;

        // Resolved textures (may be embedded or loaded from disk)
        std::shared_ptr<Texture2D> albedo;
        std::shared_ptr<Texture2D> normal;
        std::shared_ptr<Texture2D> metallicRoughness; // G=roughness, B=metallic
        std::shared_ptr<Texture2D> specular;
        std::shared_ptr<Texture2D> glossiness;

        // Fallback PBR values when no texture
        DirectX::XMFLOAT3 albedoFactor = { 1, 1, 1 };
        float             metallicFactor = 0.0f;
        float             roughnessFactor = 0.5f;

        bool isMetallicRoughness = true; // GLTF vs SpecGloss workflow
    };

    class Mesh
    {
    public:
        Mesh() = default;
        ~Mesh() = default;

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        // ---- Loading ----
        bool Load(ID3D11Device* device, const std::string& filepath,
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
        // Legacy single-material draw (for primitives)
        void Draw(ID3D11DeviceContext* ctx) const;

        // Draw a specific submesh
        void DrawSubMesh(ID3D11DeviceContext* ctx, int subMeshIndex) const;

        // ---- Data access ----
        bool IsLoaded()         const { return m_loaded; }
        int  GetIndexCount()    const { return m_totalIndexCount; }
        int  GetSubMeshCount()  const { return (int)m_subMeshes.size(); }
        int  GetMaterialCount() const { return (int)m_materials.size(); }

        const SubMesh& GetSubMesh(int i)  const { return m_subMeshes[i]; }
        const MeshMaterial& GetMaterial(int i) const { return m_materials[i]; }

        const std::vector<EmbeddedTexture>& GetEmbeddedTextures() const
        {
            return m_embeddedTextures;
        }

        bool HasEmbeddedMaterials() const { return !m_materials.empty(); }

    private:
        bool UploadSubMesh(ID3D11Device* device,
            const std::vector<Vertex3D>& vertices,
            const std::vector<uint32_t>& indices,
            int materialIndex,
            SubMesh& out);

        bool UploadSingle(ID3D11Device* device,
            const std::vector<Vertex3D>& vertices,
            const std::vector<uint32_t>& indices);

        std::vector<SubMesh>        m_subMeshes;
        std::vector<MeshMaterial>   m_materials;
        std::vector<EmbeddedTexture> m_embeddedTextures;

        // Legacy single buffer (for primitives)
        ComPtr<ID3D11Buffer> m_vertexBuffer;
        ComPtr<ID3D11Buffer> m_indexBuffer;

        int  m_totalIndexCount = 0;
        bool m_loaded = false;
        bool m_hasSubMeshes = false;

        DirectX::XMFLOAT3 m_position = { 0, 0, 0 };
        DirectX::XMFLOAT3 m_rotation = { 0, 0, 0 };
        DirectX::XMFLOAT3 m_scale = { 1, 1, 1 };

        ID3D11Device* m_device = nullptr;
    };
}
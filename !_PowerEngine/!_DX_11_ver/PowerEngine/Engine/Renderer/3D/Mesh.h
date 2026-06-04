#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <memory>
#include "Light.h"
#include "Renderer/Texture2D.h"

namespace Engine
{
    struct Vertex3D
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT2 TexCoord;
    };

    enum class UVMode
    {
        Default,
        FlipV,
        FlipU,
        FlipBoth
    };

    class Mesh
    {
    public:
        Mesh() = default;
        ~Mesh() = default;

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        bool Load(ID3D11Device* device, const std::string& filepath, UVMode uvMode = UVMode::FlipV);

        // Granular submesh controls for Renderer3D
        void DrawSubMesh(ID3D11DeviceContext* ctx, size_t index) const;
        const Material& GetMaterial(size_t subMeshIndex) const;
        uint32_t GetIndexCount() const;
        void CreatePlane(ID3D11Device* device, float width, float depth);

        // Manual material control
        void SetMaterial(int subMeshIndex, const Material& material);
        void SetMaterialAll(const Material& material);

        bool IsLoaded() const { return !m_subMeshes.empty(); }
        size_t GetSubMeshCount() const { return m_subMeshes.size(); }

        // Transform
        void SetPosition(float x, float y, float z);
        void SetRotation(float degX, float degY, float degZ);
        void SetScale(float x, float y, float z);
        void SetScale(float uniform);

        DirectX::XMMATRIX GetWorldMatrix() const;

    private:
        struct SubMesh
        {
            Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
            Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
            Material material;
            uint32_t indexCount = 0;
        };

        bool UploadSubMesh(ID3D11Device* device, SubMesh& subMesh,
            const std::vector<Vertex3D>& vertices,
            const std::vector<uint32_t>& indices);

        std::vector<SubMesh> m_subMeshes;

        // Transform
        DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 m_rotation = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 m_scale = { 1.0f, 1.0f, 1.0f };
    };
}
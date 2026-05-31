#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <string>

namespace Engine
{
    using Microsoft::WRL::ComPtr;

    struct Vertex3D
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT2 TexCoord;
    };

    class Mesh
    {
    public:
        Mesh() = default;
        ~Mesh() = default;

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        // ---- Loading ----
        bool Load(ID3D11Device* device, const std::string& filepath);
        bool CreateCube(ID3D11Device* device, float size = 1.0f);
        bool CreatePlane(ID3D11Device* device, float width = 1.0f,
            float height = 1.0f);
        bool CreateSphere(ID3D11Device* device, float radius = 1.0f,
            int slices = 16, int stacks = 16);

        // ---- Transform ----
        void SetPosition(float x, float y, float z);
        void SetRotation(float degreesX, float degreesY, float degreesZ);
        void SetScale(float x, float y, float z);
        void SetScale(float uniform);

        void Move(float dx, float dy, float dz);
        void Rotate(float degreesX, float degreesY, float degreesZ);

        DirectX::XMFLOAT3 GetPosition() const { return m_position; }
        DirectX::XMFLOAT3 GetRotation() const { return m_rotation; }
        DirectX::XMFLOAT3 GetScale()    const { return m_scale; }

        // Returns the combined world matrix
        DirectX::XMMATRIX GetWorldMatrix() const;

        // ---- Drawing ----
        void Draw(ID3D11DeviceContext* ctx) const;

        bool IsLoaded()      const { return m_loaded; }
        int  GetIndexCount() const { return m_indexCount; }

    private:
        bool Upload(ID3D11Device* device,
            const std::vector<Vertex3D>& vertices,
            const std::vector<uint32_t>& indices);

        ComPtr<ID3D11Buffer> m_vertexBuffer;
        ComPtr<ID3D11Buffer> m_indexBuffer;

        int  m_indexCount = 0;
        bool m_loaded = false;

        // Transform
        DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 m_rotation = { 0.0f, 0.0f, 0.0f }; // degrees
        DirectX::XMFLOAT3 m_scale = { 1.0f, 1.0f, 1.0f };
    };
}
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

        // Load from OBJ file
        bool LoadOBJ(ID3D11Device* device, const std::string& filepath);

        // Create primitive shapes
        bool CreateCube(ID3D11Device* device, float size = 1.0f);
        bool CreatePlane(ID3D11Device* device, float width = 1.0f,
            float height = 1.0f);

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
    };
}

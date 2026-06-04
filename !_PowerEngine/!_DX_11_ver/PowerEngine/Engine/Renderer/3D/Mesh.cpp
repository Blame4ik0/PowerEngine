#include "Mesh.h"
#include "Core/Logger.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <fstream>
#include <sstream>

using namespace DirectX;

namespace Engine
{
    // ---- Transform ----

    void Mesh::SetPosition(float x, float y, float z)
    {
        m_position = { x, y, z };
    }

    void Mesh::SetRotation(float degX, float degY, float degZ)
    {
        m_rotation = { degX, degY, degZ };
    }

    void Mesh::SetScale(float x, float y, float z)
    {
        m_scale = { x, y, z };
    }

    void Mesh::SetScale(float uniform)
    {
        m_scale = { uniform, uniform, uniform };
    }

    void Mesh::Move(float dx, float dy, float dz)
    {
        m_position.x += dx;
        m_position.y += dy;
        m_position.z += dz;
    }

    void Mesh::Rotate(float degX, float degY, float degZ)
    {
        m_rotation.x += degX;
        m_rotation.y += degY;
        m_rotation.z += degZ;
    }

    XMMATRIX Mesh::GetWorldMatrix() const
    {
        // Convert degrees to radians
        float rx = XMConvertToRadians(m_rotation.x);
        float ry = XMConvertToRadians(m_rotation.y);
        float rz = XMConvertToRadians(m_rotation.z);

        return XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z)
            * XMMatrixRotationRollPitchYaw(rx, ry, rz)
            * XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    }

    // ---- Upload ----

    bool Mesh::Upload(ID3D11Device* device,
        const std::vector<Vertex3D>& vertices,
        const std::vector<uint32_t>& indices)
    {
        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex3D) * vertices.size());
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData{};
        vbData.pSysMem = vertices.data();

        HRESULT hr = device->CreateBuffer(&vbDesc, &vbData,
            m_vertexBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Mesh: CreateBuffer (vertex) failed."); return false; }

        D3D11_BUFFER_DESC ibDesc{};
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indices.size());
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA ibData{};
        ibData.pSysMem = indices.data();

        hr = device->CreateBuffer(&ibDesc, &ibData,
            m_indexBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Mesh: CreateBuffer (index) failed."); return false; }

        m_indexCount = static_cast<int>(indices.size());
        m_loaded = true;
        return true;
    }

    // ---- Assimp loader (Improved) ----
    bool Mesh::Load(ID3D11Device* device, const std::string& filepath, UVMode uvMode)
    {
        Assimp::Importer importer;

        unsigned int flags =
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_LimitBoneWeights |
            aiProcess_FlipUVs;                    // Default: Flip V (most common)

        // Fine-tune based on file extension
        std::string ext = filepath.substr(filepath.find_last_of('.'));
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".obj")
        {
            flags |= aiProcess_FlipUVs;           // OBJ often needs UV flip
        }
        else if (ext == ".gltf" || ext == ".glb")
        {
            // GLTF usually has correct UVs
            flags &= ~aiProcess_FlipUVs;
        }
        else if (ext == ".fbx")
        {
            // FBX often needs special handling
            flags |= aiProcess_FlipUVs;
        }

        // Override with user preference
        if (uvMode == UVMode::FlipV)
            flags |= aiProcess_FlipUVs;
        else if (uvMode == UVMode::Default)
            flags &= ~aiProcess_FlipUVs;

        const aiScene* scene = importer.ReadFile(filepath, flags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            LOG_ERROR("Mesh: Assimp failed to load '{}': {}",
                filepath, importer.GetErrorString());
            return false;
        }

        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;

        // Process all meshes in the scene
        for (unsigned int m = 0; m < scene->mNumMeshes; m++)
        {
            aiMesh* mesh = scene->mMeshes[m];
            uint32_t baseVertex = static_cast<uint32_t>(vertices.size());

            for (unsigned int i = 0; i < mesh->mNumVertices; i++)
            {
                Vertex3D vert{};

                // Position
                vert.Position = {
                    mesh->mVertices[i].x,
                    mesh->mVertices[i].y,
                    mesh->mVertices[i].z
                };

                // Normal
                if (mesh->HasNormals())
                {
                    vert.Normal = {
                        mesh->mNormals[i].x,
                        mesh->mNormals[i].y,
                        mesh->mNormals[i].z
                    };
                }
                else
                {
                    vert.Normal = { 0.0f, 1.0f, 0.0f }; // fallback
                }

                // UVs
                if (mesh->mTextureCoords[0])
                {
                    float u = mesh->mTextureCoords[0][i].x;
                    float v = mesh->mTextureCoords[0][i].y;

                    // Additional manual flip if needed
                    if (uvMode == UVMode::FlipU || uvMode == UVMode::FlipBoth)
                        u = 1.0f - u;
                    if (uvMode == UVMode::FlipBoth)
                        v = 1.0f - v;

                    vert.TexCoord = { u, v };
                }
                else
                {
                    vert.TexCoord = { 0.0f, 0.0f };
                }

                vertices.push_back(vert);
            }

            // Indices
            for (unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace& face = mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                {
                    indices.push_back(baseVertex + face.mIndices[j]);
                }
            }
        }

        LOG_INFO("Mesh loaded successfully: '{}' ({} vertices, {} indices, {} meshes)",
            filepath, vertices.size(), indices.size(), scene->mNumMeshes);

        return Upload(device, vertices, indices);
    }

    // ---- Primitives ----

    bool Mesh::CreateCube(ID3D11Device* device, float s)
    {
        float h = s * 0.5f;

        std::vector<Vertex3D> vertices =
        {
            {{ -h, -h, -h }, { 0,  0, -1 }, { 0, 1 }},
            {{ -h,  h, -h }, { 0,  0, -1 }, { 0, 0 }},
            {{  h,  h, -h }, { 0,  0, -1 }, { 1, 0 }},
            {{  h, -h, -h }, { 0,  0, -1 }, { 1, 1 }},

            {{  h, -h,  h }, { 0,  0,  1 }, { 0, 1 }},
            {{  h,  h,  h }, { 0,  0,  1 }, { 0, 0 }},
            {{ -h,  h,  h }, { 0,  0,  1 }, { 1, 0 }},
            {{ -h, -h,  h }, { 0,  0,  1 }, { 1, 1 }},

            {{ -h, -h,  h }, { -1, 0,  0 }, { 0, 1 }},
            {{ -h,  h,  h }, { -1, 0,  0 }, { 0, 0 }},
            {{ -h,  h, -h }, { -1, 0,  0 }, { 1, 0 }},
            {{ -h, -h, -h }, { -1, 0,  0 }, { 1, 1 }},

            {{  h, -h, -h }, {  1, 0,  0 }, { 0, 1 }},
            {{  h,  h, -h }, {  1, 0,  0 }, { 0, 0 }},
            {{  h,  h,  h }, {  1, 0,  0 }, { 1, 0 }},
            {{  h, -h,  h }, {  1, 0,  0 }, { 1, 1 }},

            {{ -h,  h, -h }, {  0, 1,  0 }, { 0, 1 }},
            {{ -h,  h,  h }, {  0, 1,  0 }, { 0, 0 }},
            {{  h,  h,  h }, {  0, 1,  0 }, { 1, 0 }},
            {{  h,  h, -h }, {  0, 1,  0 }, { 1, 1 }},

            {{ -h, -h,  h }, {  0, -1, 0 }, { 0, 1 }},
            {{ -h, -h, -h }, {  0, -1, 0 }, { 0, 0 }},
            {{  h, -h, -h }, {  0, -1, 0 }, { 1, 0 }},
            {{  h, -h,  h }, {  0, -1, 0 }, { 1, 1 }},
        };

        std::vector<uint32_t> indices;
        for (uint32_t i = 0; i < 6; i++)
        {
            uint32_t b = i * 4;
            indices.push_back(b);     indices.push_back(b + 1);
            indices.push_back(b + 2);
            indices.push_back(b);     indices.push_back(b + 2);
            indices.push_back(b + 3);
        }

        LOG_INFO("Mesh: cube created.");
        return Upload(device, vertices, indices);
    }

    bool Mesh::CreatePlane(ID3D11Device* device, float width, float height)
    {
        float hw = width * 0.5f;
        float hh = height * 0.5f;

        std::vector<Vertex3D> vertices =
        {
            {{ -hw, 0, -hh }, { 0, 1, 0 }, { 0, 1 }},
            {{ -hw, 0,  hh }, { 0, 1, 0 }, { 0, 0 }},
            {{  hw, 0,  hh }, { 0, 1, 0 }, { 1, 0 }},
            {{  hw, 0, -hh }, { 0, 1, 0 }, { 1, 1 }},
        };

        std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };

        LOG_INFO("Mesh: plane created.");
        return Upload(device, vertices, indices);
    }

    bool Mesh::CreateSphere(ID3D11Device* device,
        float radius, int slices, int stacks)
    {
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;

        for (int i = 0; i <= stacks; i++)
        {
            float phi = XM_PI * i / stacks;
            for (int j = 0; j <= slices; j++)
            {
                float theta = XM_2PI * j / slices;

                Vertex3D v{};
                v.Position = {
                    radius * sinf(phi) * cosf(theta),
                    radius * cosf(phi),
                    radius * sinf(phi) * sinf(theta)
                };
                v.Normal = {
                    sinf(phi) * cosf(theta),
                    cosf(phi),
                    sinf(phi) * sinf(theta)
                };
                v.TexCoord = {
                    static_cast<float>(j) / slices,
                    static_cast<float>(i) / stacks
                };
                vertices.push_back(v);
            }
        }

        for (int i = 0; i < stacks; i++)
        {
            for (int j = 0; j < slices; j++)
            {
                uint32_t a = i * (slices + 1) + j;
                uint32_t b = a + slices + 1;

                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(a + 1);

                indices.push_back(b);
                indices.push_back(b + 1);
                indices.push_back(a + 1);
            }
        }

        LOG_INFO("Mesh: sphere created ({} slices, {} stacks).", slices, stacks);
        return Upload(device, vertices, indices);
    }

    void Mesh::Draw(ID3D11DeviceContext* ctx) const
    {
        if (!m_loaded) return;

        UINT stride = sizeof(Vertex3D);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
        ctx->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->DrawIndexed(static_cast<UINT>(m_indexCount), 0, 0);
    }
}
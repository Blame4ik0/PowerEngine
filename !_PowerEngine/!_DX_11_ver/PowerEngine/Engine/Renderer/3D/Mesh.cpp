#include "Mesh.h"
#include "Core/Logger.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace DirectX;

namespace Engine
{
    // ====================== TRANSFORM ======================
    void Mesh::SetPosition(float x, float y, float z) { m_position = { x, y, z }; }
    void Mesh::SetRotation(float degX, float degY, float degZ) { m_rotation = { degX, degY, degZ }; }
    void Mesh::SetScale(float x, float y, float z) { m_scale = { x, y, z }; }
    void Mesh::SetScale(float uniform) { m_scale = { uniform, uniform, uniform }; }

    XMMATRIX Mesh::GetWorldMatrix() const
    {
        float rx = XMConvertToRadians(m_rotation.x);
        float ry = XMConvertToRadians(m_rotation.y);
        float rz = XMConvertToRadians(m_rotation.z);

        return XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z) *
            XMMatrixRotationRollPitchYaw(rx, ry, rz) *
            XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    }

    // ====================== UPLOAD ======================
    bool Mesh::UploadSubMesh(ID3D11Device* device, SubMesh& subMesh,
        const std::vector<Vertex3D>& vertices,
        const std::vector<uint32_t>& indices)
    {
        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = sizeof(Vertex3D) * static_cast<UINT>(vertices.size());
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData{};
        vbData.pSysMem = vertices.data();

        HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, subMesh.vertexBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        D3D11_BUFFER_DESC ibDesc{};
        ibDesc.ByteWidth = sizeof(uint32_t) * static_cast<UINT>(indices.size());
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA ibData{};
        ibData.pSysMem = indices.data();

        hr = device->CreateBuffer(&ibDesc, &ibData, subMesh.indexBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        subMesh.indexCount = static_cast<uint32_t>(indices.size());
        return true;
    }

    // ====================== MATERIAL PARSING ======================
    Material ParseMaterial(ID3D11Device* device, aiMaterial* aiMat, const aiScene* scene, const std::string& directory)
    {
        Material mat;

        aiColor4D diffuse(1.0f, 1.0f, 1.0f, 1.0f);
        aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &diffuse);
        mat.Albedo = { diffuse.r, diffuse.g, diffuse.b };

        aiString texPath;
        if (aiGetMaterialTexture(aiMat, aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
        {
            if (texPath.C_Str()[0] == '*') // Embedded texture (.glb)
            {
                int idx = std::atoi(texPath.C_Str() + 1);
                if (idx < (int)scene->mNumTextures)
                {
                    aiTexture* tex = scene->mTextures[idx];
                    auto embedded = std::make_shared<Texture2D>();
                    if (tex->mHeight == 0 && embedded->LoadFromMemory(device,
                        reinterpret_cast<const unsigned char*>(tex->pcData),
                        tex->mWidth, tex->mHeight))
                    {
                        mat.AlbedoTexture = embedded;
                        LOG_INFO("Loaded embedded albedo texture");
                    }
                }
            }
            else // External texture (.obj / .fbx)
            {
                mat.AlbedoMap = directory + texPath.C_Str();
            }
        }

        mat.Metallic = 0.0f;
        mat.Roughness = 0.6f;
        mat.AmbientOcclusion = 1.0f;

        return mat;
    }

    // ====================== LOAD ======================
    bool Mesh::Load(ID3D11Device* device, const std::string& filepath, UVMode uvMode)
    {
        Assimp::Importer importer;

        unsigned int flags = aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_FlipUVs;

        const aiScene* scene = importer.ReadFile(filepath, flags);

        if (!scene || !scene->mRootNode)
        {
            LOG_ERROR("Failed to load {}: {}", filepath, importer.GetErrorString());
            return false;
        }

        std::string directory = "";
        size_t lastSlash = filepath.find_last_of("/\\");
        if (lastSlash != std::string::npos)
        {
            directory = filepath.substr(0, lastSlash + 1);
        }

        m_subMeshes.clear();

        for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
        {
            aiMesh* aiMesh = scene->mMeshes[m];
            aiMaterial* aiMat = scene->mMaterials[aiMesh->mMaterialIndex];

            SubMesh subMesh;
            subMesh.material = ParseMaterial(device, aiMat, scene, directory);

            std::vector<Vertex3D> vertices;
            std::vector<uint32_t> indices;

            for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i)
            {
                Vertex3D v{};
                v.Position = { aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z };
                if (aiMesh->HasNormals())
                    v.Normal = { aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z };
                if (aiMesh->mTextureCoords[0])
                    v.TexCoord = { aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y };

                vertices.push_back(v);
            }

            for (unsigned int i = 0; i < aiMesh->mNumFaces; ++i)
            {
                aiFace& face = aiMesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; ++j)
                    indices.push_back(face.mIndices[j]);
            }

            if (!UploadSubMesh(device, subMesh, vertices, indices))
            {
                LOG_ERROR("Failed to upload submesh");
                return false;
            }

            m_subMeshes.push_back(std::move(subMesh));
        }

        LOG_INFO("Loaded '{}' with {} submeshes", filepath, m_subMeshes.size());
        return true;
    }

    // ====================== ENGINE EXTENSIONS ======================
    void Mesh::CreatePlane(ID3D11Device* device, float width, float depth)
    {
        m_subMeshes.clear();

        std::vector<Vertex3D> vertices = {
            { {-width / 2.0f, 0.0f,  depth / 2.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
            { { width / 2.0f, 0.0f,  depth / 2.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
            { { width / 2.0f, 0.0f, -depth / 2.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },
            { {-width / 2.0f, 0.0f, -depth / 2.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} }
        };

        std::vector<uint32_t> indices = {
            0, 1, 2,
            0, 2, 3
        };

        SubMesh subMesh;
        subMesh.material.Albedo = { 1.0f, 1.0f, 1.0f };
        subMesh.material.Metallic = 0.0f;
        subMesh.material.Roughness = 1.0f;
        subMesh.material.AmbientOcclusion = 1.0f;

        if (UploadSubMesh(device, subMesh, vertices, indices))
        {
            m_subMeshes.push_back(std::move(subMesh));
        }
    }

    uint32_t Mesh::GetIndexCount() const
    {
        uint32_t totalIndices = 0;
        for (const auto& sub : m_subMeshes)
        {
            totalIndices += sub.indexCount;
        }
        return totalIndices;
    }

    const Material& Mesh::GetMaterial(size_t subMeshIndex) const
    {
        return m_subMeshes[subMeshIndex].material;
    }

    void Mesh::DrawSubMesh(ID3D11DeviceContext* ctx, size_t index) const
    {
        if (index >= m_subMeshes.size()) return;
        const auto& sub = m_subMeshes[index];
        if (sub.indexCount == 0) return;

        UINT stride = sizeof(Vertex3D);
        UINT offset = 0;

        ctx->IASetVertexBuffers(0, 1, sub.vertexBuffer.GetAddressOf(), &stride, &offset);
        ctx->IASetIndexBuffer(sub.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        ctx->DrawIndexed(sub.indexCount, 0, 0);
    }

    // ====================== MANUAL OVERRIDE ======================
    void Mesh::SetMaterial(int subMeshIndex, const Material& material)
    {
        if (subMeshIndex >= 0 && subMeshIndex < (int)m_subMeshes.size())
            m_subMeshes[subMeshIndex].material = material;
    }

    void Mesh::SetMaterialAll(const Material& material)
    {
        for (auto& sub : m_subMeshes)
            sub.material = material;
    }
}
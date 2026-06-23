#include "Mesh.h"
#include "Renderer/Texture2D.h"
#include "Core/Logger.h"

#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/pbrmaterial.h>

#include <algorithm>

using namespace DirectX;

namespace Engine
{
    // ---- Transform ----

    void Mesh::SetPosition(float x, float y, float z) { m_position = { x, y, z }; }
    void Mesh::SetRotation(float x, float y, float z) { m_rotation = { x, y, z }; }
    void Mesh::SetScale(float x, float y, float z) { m_scale = { x, y, z }; }
    void Mesh::SetScale(float u) { m_scale = { u, u, u }; }

    void Mesh::Move(float dx, float dy, float dz)
    {
        m_position.x += dx;
        m_position.y += dy;
        m_position.z += dz;
    }

    void Mesh::Rotate(float dx, float dy, float dz)
    {
        m_rotation.x += dx;
        m_rotation.y += dy;
        m_rotation.z += dz;
    }

    XMMATRIX Mesh::GetWorldMatrix() const
    {
        return XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z)
            * XMMatrixRotationRollPitchYaw(
                XMConvertToRadians(m_rotation.x),
                XMConvertToRadians(m_rotation.y),
                XMConvertToRadians(m_rotation.z))
            * XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    }

    // ---- AABB ----

    void AABB::GetCorners(const XMMATRIX& world, XMFLOAT3 out[8]) const
    {
        XMFLOAT3 corners[8] =
        {
            { min.x, min.y, min.z }, { max.x, min.y, min.z },
            { min.x, max.y, min.z }, { max.x, max.y, min.z },
            { min.x, min.y, max.z }, { max.x, min.y, max.z },
            { min.x, max.y, max.z }, { max.x, max.y, max.z },
        };
        for (int i = 0; i < 8; i++)
        {
            XMVECTOR v = XMVector3TransformCoord(
                XMLoadFloat3(&corners[i]), world);
            XMStoreFloat3(&out[i], v);
        }
    }

    void Mesh::ComputeAABB(const std::vector<Vertex3D>& vertices)
    {
        m_aabb = {};
        ExpandAABB(vertices);
    }

    void Mesh::ExpandAABB(const std::vector<Vertex3D>& vertices)
    {
        for (auto& v : vertices)
        {
            m_aabb.min.x = std::min(m_aabb.min.x, v.Position.x);
            m_aabb.min.y = std::min(m_aabb.min.y, v.Position.y);
            m_aabb.min.z = std::min(m_aabb.min.z, v.Position.z);
            m_aabb.max.x = std::max(m_aabb.max.x, v.Position.x);
            m_aabb.max.y = std::max(m_aabb.max.y, v.Position.y);
            m_aabb.max.z = std::max(m_aabb.max.z, v.Position.z);
        }
    }

    // ---- Upload helpers ----

    bool Mesh::UploadSubMesh(ID3D11Device* device,
        const std::vector<Vertex3D>& vertices,
        const std::vector<uint32_t>& indices,
        int materialIndex,
        SubMesh& out)
    {
        ExpandAABB(vertices);

        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex3D) * vertices.size());
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData{ vertices.data() };
        if (FAILED(device->CreateBuffer(&vbDesc, &vbData,
            out.vertexBuffer.GetAddressOf())))
            return false;

        D3D11_BUFFER_DESC ibDesc{};
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indices.size());
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA ibData{ indices.data() };
        if (FAILED(device->CreateBuffer(&ibDesc, &ibData,
            out.indexBuffer.GetAddressOf())))
            return false;

        out.indexCount = static_cast<int>(indices.size());
        out.materialIndex = materialIndex;
        return true;
    }

    bool Mesh::UploadSingle(ID3D11Device* device,
        const std::vector<Vertex3D>& vertices,
        const std::vector<uint32_t>& indices)
    {
        ComputeAABB(vertices);

        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex3D) * vertices.size());
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vd{ vertices.data() };
        if (FAILED(device->CreateBuffer(&vbDesc, &vd,
            m_vertexBuffer.GetAddressOf()))) return false;

        D3D11_BUFFER_DESC ibDesc{};
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indices.size());
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA id_{ indices.data() };
        if (FAILED(device->CreateBuffer(&ibDesc, &id_,
            m_indexBuffer.GetAddressOf()))) return false;

        m_totalIndexCount = static_cast<int>(indices.size());
        return true;
    }

    // ---- Main loader ----

    bool Mesh::Load(ID3D11Device* device, ID3D11DeviceContext* ctx,
        const std::string& filepath,
        UVMode uvMode)
    {
        m_device = device;
        m_aabb = {}; // reset — UploadSubMesh will accumulate bounds across all submeshes

        Assimp::Importer importer;

        unsigned int flags =
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices;

        if (uvMode == UVMode::FlipV)
            flags |= aiProcess_FlipUVs;

        const aiScene* scene = importer.ReadFile(filepath, flags);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            LOG_ERROR("Mesh: Assimp error for '{}': {}",
                filepath, importer.GetErrorString());
            return false;
        }

        // ---- Extract embedded textures ----
        m_embeddedTextures.clear();
        for (unsigned int i = 0; i < scene->mNumTextures; i++)
        {
            aiTexture* aiTex = scene->mTextures[i];
            auto tex = std::make_shared<Texture2D>();
            if (tex->LoadFromAssimp(device, ctx, aiTex))
            {
                EmbeddedTexture et;
                et.name = aiTex->mFilename.C_Str();
                et.texture = tex;
                m_embeddedTextures.push_back(et);
            }
        }

        if (scene->mNumTextures > 0)
            LOG_INFO("Mesh: loaded {} embedded textures from '{}'.",
                scene->mNumTextures, filepath);

        // ---- Extract materials ----
        m_materials.clear();
        for (unsigned int i = 0; i < scene->mNumMaterials; i++)
        {
            aiMaterial* aiMat = scene->mMaterials[i];
            MeshMaterial mat;

            aiString matName;
            aiMat->Get(AI_MATKEY_NAME, matName);
            mat.name = matName.C_Str();

            // Helper: resolve embedded or external texture
            auto resolveTexture = [&](const std::string& texPathStr) -> std::shared_ptr<Texture2D>
                {
                    std::string path = texPathStr;
                    if (path.empty()) return nullptr;

                    // Embedded texture reference — format is "*N"
                    if (path[0] == '*')
                    {
                        int idx = std::atoi(path.c_str() + 1);
                        if (idx >= 0 && idx < (int)m_embeddedTextures.size())
                            return m_embeddedTextures[idx].texture;
                        return nullptr;
                    }

                    // External texture
                    std::string modelDir = filepath.substr(0, filepath.find_last_of("/\\") + 1);
                    std::string fullPath = modelDir + path;

                    auto tex = std::make_shared<Texture2D>();
                    if (tex->Load(device, ctx, fullPath))
                        return tex;

                    LOG_WARN("Mesh: could not load external texture '{}'.", fullPath);
                    return nullptr;
                };

            // Helper: get texture by standard type
            auto getTexture = [&](aiTextureType type) -> std::shared_ptr<Texture2D>
                {
                    if (aiMat->GetTextureCount(type) == 0) return nullptr;

                    aiString texPath;
                    aiMat->GetTexture(type, 0, &texPath);
                    return resolveTexture(texPath.C_Str());
                };

            // Detect workflow
            int shadingModel = 0;
            aiMat->Get(AI_MATKEY_SHADING_MODEL, shadingModel);
            mat.isMetallicRoughness = (shadingModel == aiShadingMode_PBR_BRDF);

            // Albedo / base color
            mat.albedo = getTexture(aiTextureType_DIFFUSE);
            if (!mat.albedo)
                mat.albedo = getTexture(aiTextureType_BASE_COLOR);

            // Normal map
            mat.normal = getTexture(aiTextureType_NORMALS);
            if (!mat.normal)
                mat.normal = getTexture(aiTextureType_HEIGHT);

            // Metallic-Roughness (GLTF PBR)
            {
                aiString texPath;
                if (aiMat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &texPath) == AI_SUCCESS)
                {
                    mat.metallicRoughness = resolveTexture(texPath.C_Str());
                }
                else
                {
                    // Fallback for separate maps
                    if (aiMat->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &texPath) == AI_SUCCESS ||
                        aiMat->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &texPath) == AI_SUCCESS)
                    {
                        mat.metallicRoughness = resolveTexture(texPath.C_Str());
                    }
                }
            }

            // Specular / Glossiness (older workflow)
            mat.specular = getTexture(aiTextureType_SPECULAR);
            mat.glossiness = getTexture(aiTextureType_SHININESS);

            // Material factors
            aiColor4D baseColor(1, 1, 1, 1);
            aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);
            mat.albedoFactor = { baseColor.r, baseColor.g, baseColor.b };

            aiMat->Get(AI_MATKEY_METALLIC_FACTOR, mat.metallicFactor);
            aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, mat.roughnessFactor);

            m_materials.push_back(mat);
        }

        LOG_INFO("Mesh: loaded {} materials from '{}'.",
            m_materials.size(), filepath);

        // ---- Extract submeshes ----
        m_subMeshes.clear();
        m_totalIndexCount = 0;

        for (unsigned int m = 0; m < scene->mNumMeshes; m++)
        {
            aiMesh* mesh = scene->mMeshes[m];

            std::vector<Vertex3D> vertices;
            std::vector<uint32_t> indices;

            for (unsigned int i = 0; i < mesh->mNumVertices; i++)
            {
                Vertex3D v{};
                v.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
                if (mesh->HasNormals())
                    v.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
                if (mesh->HasTangentsAndBitangents())
                    v.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
                if (mesh->mTextureCoords[0])
                {
                    float u = mesh->mTextureCoords[0][i].x;
                    float v2 = mesh->mTextureCoords[0][i].y;
                    if (uvMode == UVMode::FlipU || uvMode == UVMode::FlipBoth)
                        u = 1.0f - u;
                    if (uvMode == UVMode::FlipBoth)
                        v2 = 1.0f - v2;
                    v.TexCoord = { u, v2 };
                }
                vertices.push_back(v);
            }

            for (unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace& face = mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }

            SubMesh sub;
            if (!UploadSubMesh(device, vertices, indices, (int)mesh->mMaterialIndex, sub))
            {
                LOG_ERROR("Mesh: failed to upload submesh {} of '{}'.", m, filepath);
                continue;
            }

            m_totalIndexCount += sub.indexCount;
            m_subMeshes.push_back(std::move(sub));
        }

        m_hasSubMeshes = true;
        m_loaded = true;

        LOG_INFO("Mesh loaded: '{}' ({} submeshes, {} total indices).",
            filepath, m_subMeshes.size(), m_totalIndexCount);
        return true;
    }

    // ---- Draw functions (unchanged) ----
    void Mesh::Draw(ID3D11DeviceContext* ctx) const
    {
        if (!m_loaded) return;

        if (m_hasSubMeshes)
        {
            for (auto& sub : m_subMeshes)
            {
                UINT stride = sizeof(Vertex3D);
                UINT offset = 0;
                ctx->IASetVertexBuffers(0, 1, sub.vertexBuffer.GetAddressOf(), &stride, &offset);
                ctx->IASetIndexBuffer(sub.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
                ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                ctx->DrawIndexed(sub.indexCount, 0, 0);
            }
            return;
        }

        // Legacy single buffer path
        UINT stride = sizeof(Vertex3D);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
        ctx->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->DrawIndexed(m_totalIndexCount, 0, 0);
    }

    void Mesh::DrawSubMesh(ID3D11DeviceContext* ctx, int i) const
    {
        if (!m_loaded || i < 0 || i >= (int)m_subMeshes.size()) return;
        const SubMesh& sub = m_subMeshes[i];
        UINT stride = sizeof(Vertex3D);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, sub.vertexBuffer.GetAddressOf(), &stride, &offset);
        ctx->IASetIndexBuffer(sub.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->DrawIndexed(sub.indexCount, 0, 0);
    }

    // ---- Primitive creation functions ----
    bool Mesh::CreateCube(ID3D11Device* device, float s)
    {
        float h = s * 0.5f;
        std::vector<Vertex3D> vertices =
        {
            // +Z  normal(0,0,1)   tangent(1,0,0)
            {{-h,-h, h},{0,0,1},{0,1},{1,0,0}},
            {{-h, h, h},{0,0,1},{0,0},{1,0,0}},
            {{ h, h, h},{0,0,1},{1,0},{1,0,0}},
            {{ h,-h, h},{0,0,1},{1,1},{1,0,0}},
            // -Z  normal(0,0,-1)  tangent(-1,0,0)
            {{ h,-h,-h},{0,0,-1},{0,1},{-1,0,0}},
            {{ h, h,-h},{0,0,-1},{0,0},{-1,0,0}},
            {{-h, h,-h},{0,0,-1},{1,0},{-1,0,0}},
            {{-h,-h,-h},{0,0,-1},{1,1},{-1,0,0}},
            // +X  normal(1,0,0)   tangent(0,0,-1)
            {{ h,-h, h},{1,0,0},{0,1},{0,0,-1}},
            {{ h, h, h},{1,0,0},{0,0},{0,0,-1}},
            {{ h, h,-h},{1,0,0},{1,0},{0,0,-1}},
            {{ h,-h,-h},{1,0,0},{1,1},{0,0,-1}},
            // -X  normal(-1,0,0)  tangent(0,0,1)
            {{-h,-h,-h},{-1,0,0},{0,1},{0,0,1}},
            {{-h, h,-h},{-1,0,0},{0,0},{0,0,1}},
            {{-h, h, h},{-1,0,0},{1,0},{0,0,1}},
            {{-h,-h, h},{-1,0,0},{1,1},{0,0,1}},
            // +Y  normal(0,1,0)   tangent(1,0,0)
            {{-h, h, h},{0,1,0},{0,1},{1,0,0}},
            {{-h, h,-h},{0,1,0},{0,0},{1,0,0}},
            {{ h, h,-h},{0,1,0},{1,0},{1,0,0}},
            {{ h, h, h},{0,1,0},{1,1},{1,0,0}},
            // -Y  normal(0,-1,0)  tangent(1,0,0)
            {{-h,-h,-h},{0,-1,0},{0,1},{1,0,0}},
            {{-h,-h, h},{0,-1,0},{0,0},{1,0,0}},
            {{ h,-h, h},{0,-1,0},{1,0},{1,0,0}},
            {{ h,-h,-h},{0,-1,0},{1,1},{1,0,0}},
        };
        std::vector<uint32_t> indices;
        for (uint32_t i = 0; i < 6; i++)
        {
            uint32_t b = i * 4;
            indices.insert(indices.end(), { b,b + 1,b + 2, b,b + 2,b + 3 });
        }
        m_hasSubMeshes = false;
        m_loaded = UploadSingle(device, vertices, indices);
        if (m_loaded) LOG_INFO("Mesh: cube created.");
        return m_loaded;
    }

    bool Mesh::CreatePlane(ID3D11Device* device, float width, float height)
    {
        float hw = width * 0.5f, hh = height * 0.5f;
        // Tangent for a Y-up plane is +X
        std::vector<Vertex3D> vertices =
        {
            {{ -hw, 0,-hh },{ 0,1,0 },{ 0,1 },{ 1,0,0 }},
            {{ -hw, 0, hh },{ 0,1,0 },{ 0,0 },{ 1,0,0 }},
            {{  hw, 0, hh },{ 0,1,0 },{ 1,0 },{ 1,0,0 }},
            {{  hw, 0,-hh },{ 0,1,0 },{ 1,1 },{ 1,0,0 }},
        };
        std::vector<uint32_t> indices = { 0,1,2, 0,2,3 };
        m_hasSubMeshes = false;
        m_loaded = UploadSingle(device, vertices, indices);
        if (m_loaded) LOG_INFO("Mesh: plane created.");
        return m_loaded;
    }

    bool Mesh::CreateSphere(ID3D11Device* device, float radius, int slices, int stacks)
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
                v.Position = { radius * sinf(phi) * cosf(theta), radius * cosf(phi), radius * sinf(phi) * sinf(theta) };
                v.Normal = { sinf(phi) * cosf(theta), cosf(phi), sinf(phi) * sinf(theta) };
                v.TexCoord = { (float)j / slices, (float)i / stacks };
                // Tangent = d(position)/d(theta), normalized
                v.Tangent = { -sinf(theta), 0.f, cosf(theta) };
                vertices.push_back(v);
            }
        }
        for (int i = 0; i < stacks; i++)
        {
            for (int j = 0; j < slices; j++)
            {
                uint32_t a = i * (slices + 1) + j;
                uint32_t b = a + slices + 1;
                indices.insert(indices.end(), { a, b, a + 1, b, b + 1, a + 1 });
            }
        }
        m_hasSubMeshes = false;
        m_loaded = UploadSingle(device, vertices, indices);
        if (m_loaded) LOG_INFO("Mesh: sphere created ({} slices, {} stacks).", slices, stacks);
        return m_loaded;
    }

    // ---- Material overrides ----

    void Mesh::BindBuffers(ID3D11DeviceContext* ctx,
        ID3D11Buffer* instanceBuffer,
        int instanceCount) const
    {
        if (!m_vertexBuffer || !m_indexBuffer) return;

        ID3D11Buffer* vbs[2] = { m_vertexBuffer.Get(), instanceBuffer };
        UINT strides[2] = { sizeof(Vertex3D), sizeof(XMFLOAT4X4) };
        UINT offsets[2] = { 0, 0 };
        ctx->IASetVertexBuffers(0, 2, vbs, strides, offsets);
        ctx->IASetIndexBuffer(m_indexBuffer.Get(),
            DXGI_FORMAT_R32_UINT, 0);
        ctx->DrawIndexedInstanced(
            (UINT)m_totalIndexCount, (UINT)instanceCount, 0, 0, 0);
    }

    void Mesh::SetMaterialOverride(const MaterialOverride & override)
    {
        m_globalOverride = override;
        m_hasGlobalOverride = true;
    }

    void Mesh::SetMaterialOverride(int materialIndex, const MaterialOverride & override)
    {
        m_materialOverrides[materialIndex] = override;
    }

    void Mesh::ClearMaterialOverrides()
    {
        m_hasGlobalOverride = false;
        m_globalOverride = {};
        m_materialOverrides.clear();
    }

    const MaterialOverride* Mesh::ResolveOverride(int materialIndex) const
    {
        // Per-material override wins over global
        auto it = m_materialOverrides.find(materialIndex);
        if (it != m_materialOverrides.end())
            return &it->second;

        if (m_hasGlobalOverride)
            return &m_globalOverride;

        return nullptr;
    }
}
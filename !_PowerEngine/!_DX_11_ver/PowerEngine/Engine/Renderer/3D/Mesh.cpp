#include "Mesh.h"
#include "Core/Logger.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>

using namespace DirectX;

namespace Engine
{
    bool Mesh::Upload(ID3D11Device* device,
        const std::vector<Vertex3D>& vertices,
        const std::vector<uint32_t>& indices)
    {
        // ---- Vertex buffer ----
        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex3D) * vertices.size());
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData{};
        vbData.pSysMem = vertices.data();

        HRESULT hr = device->CreateBuffer(&vbDesc, &vbData,
            m_vertexBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Mesh: CreateBuffer (vertex) failed."); return false; }

        // ---- Index buffer ----
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

    bool Mesh::LoadOBJ(ID3D11Device* device, const std::string& filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            LOG_ERROR("Mesh: could not open '{}'.", filepath);
            return false;
        }

        std::vector<XMFLOAT3> positions;
        std::vector<XMFLOAT3> normals;
        std::vector<XMFLOAT2> texcoords;

        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;

        // Simple cache to avoid duplicate vertices
        std::unordered_map<std::string, uint32_t> vertexCache;

        std::string line;
        while (std::getline(file, line))
        {
            std::istringstream ss(line);
            std::string token;
            ss >> token;

            if (token == "v")
            {
                XMFLOAT3 pos;
                ss >> pos.x >> pos.y >> pos.z;
                positions.push_back(pos);
            }
            else if (token == "vn")
            {
                XMFLOAT3 normal;
                ss >> normal.x >> normal.y >> normal.z;
                normals.push_back(normal);
            }
            else if (token == "vt")
            {
                XMFLOAT2 uv;
                ss >> uv.x >> uv.y;
                uv.y = 1.0f - uv.y; // Flip V — OBJ is bottom-left origin, DX is top-left
                texcoords.push_back(uv);
            }
            else if (token == "f")
            {
                // Face — can be triangle or quad
                // Format: position/texcoord/normal (indices are 1-based in OBJ)
                std::vector<uint32_t> faceIndices;
                std::string vertStr;

                while (ss >> vertStr)
                {
                    // Check cache first
                    auto it = vertexCache.find(vertStr);
                    if (it != vertexCache.end())
                    {
                        faceIndices.push_back(it->second);
                        continue;
                    }

                    // Parse p/t/n or p//n or p
                    int pi = 0, ti = 0, ni = 0;
                    std::replace(vertStr.begin(), vertStr.end(), '/', ' ');
                    std::istringstream vs(vertStr);
                    vs >> pi;
                    if (vs.peek() == ' ') { vs >> ti; }
                    if (vs.peek() == ' ') { vs >> ni; }

                    Vertex3D vert{};
                    if (pi > 0 && pi <= (int)positions.size())
                        vert.Position = positions[pi - 1];
                    if (ti > 0 && ti <= (int)texcoords.size())
                        vert.TexCoord = texcoords[ti - 1];
                    if (ni > 0 && ni <= (int)normals.size())
                        vert.Normal = normals[ni - 1];

                    uint32_t idx = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vert);
                    vertexCache[vertStr] = idx;
                    faceIndices.push_back(idx);
                }

                // Triangulate — fan triangulation for quads
                for (size_t i = 1; i + 1 < faceIndices.size(); i++)
                {
                    indices.push_back(faceIndices[0]);
                    indices.push_back(faceIndices[i]);
                    indices.push_back(faceIndices[i + 1]);
                }
            }
        }

        if (vertices.empty())
        {
            LOG_ERROR("Mesh: no geometry found in '{}'.", filepath);
            return false;
        }

        LOG_INFO("Mesh loaded: '{}' ({} vertices, {} indices).",
            filepath, vertices.size(), indices.size());

        return Upload(device, vertices, indices);
    }

    bool Mesh::CreateCube(ID3D11Device* device, float s)
    {
        float h = s * 0.5f;

        std::vector<Vertex3D> vertices =
        {
            // Front face  (normal  0, 0,-1)
            {{ -h, -h, -h }, { 0, 0, -1 }, { 0, 1 }},
            {{ -h,  h, -h }, { 0, 0, -1 }, { 0, 0 }},
            {{  h,  h, -h }, { 0, 0, -1 }, { 1, 0 }},
            {{  h, -h, -h }, { 0, 0, -1 }, { 1, 1 }},

            // Back face   (normal  0, 0, 1)
            {{  h, -h,  h }, { 0, 0,  1 }, { 0, 1 }},
            {{  h,  h,  h }, { 0, 0,  1 }, { 0, 0 }},
            {{ -h,  h,  h }, { 0, 0,  1 }, { 1, 0 }},
            {{ -h, -h,  h }, { 0, 0,  1 }, { 1, 1 }},

            // Left face   (normal -1, 0, 0)
            {{ -h, -h,  h }, { -1, 0, 0 }, { 0, 1 }},
            {{ -h,  h,  h }, { -1, 0, 0 }, { 0, 0 }},
            {{ -h,  h, -h }, { -1, 0, 0 }, { 1, 0 }},
            {{ -h, -h, -h }, { -1, 0, 0 }, { 1, 1 }},

            // Right face  (normal  1, 0, 0)
            {{  h, -h, -h }, { 1, 0, 0 }, { 0, 1 }},
            {{  h,  h, -h }, { 1, 0, 0 }, { 0, 0 }},
            {{  h,  h,  h }, { 1, 0, 0 }, { 1, 0 }},
            {{  h, -h,  h }, { 1, 0, 0 }, { 1, 1 }},

            // Top face    (normal  0, 1, 0)
            {{ -h,  h, -h }, { 0, 1, 0 }, { 0, 1 }},
            {{ -h,  h,  h }, { 0, 1, 0 }, { 0, 0 }},
            {{  h,  h,  h }, { 0, 1, 0 }, { 1, 0 }},
            {{  h,  h, -h }, { 0, 1, 0 }, { 1, 1 }},

            // Bottom face (normal  0,-1, 0)
            {{ -h, -h,  h }, { 0, -1, 0 }, { 0, 1 }},
            {{ -h, -h, -h }, { 0, -1, 0 }, { 0, 0 }},
            {{  h, -h, -h }, { 0, -1, 0 }, { 1, 0 }},
            {{  h, -h,  h }, { 0, -1, 0 }, { 1, 1 }},
        };

        std::vector<uint32_t> indices;
        for (uint32_t i = 0; i < 6; i++)
        {
            uint32_t base = i * 4;
            indices.push_back(base);     indices.push_back(base + 1);
            indices.push_back(base + 2);
            indices.push_back(base);     indices.push_back(base + 2);
            indices.push_back(base + 3);
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

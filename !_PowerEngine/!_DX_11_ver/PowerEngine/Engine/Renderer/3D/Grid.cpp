#include "Grid.h"
#include "Renderer3D.h"
#include "Light.h"
#include "Core/Logger.h"

using namespace DirectX;

namespace Engine
{
    struct GridVertex
    {
        XMFLOAT3 Position;
        XMFLOAT4 Color;
    };

    bool Grid::Init(RenderContext* context,
        const std::wstring& gridShaderPath,
        Renderer3D* renderer3D,
        int size, float spacing)
    {
        m_context = context;
        m_renderer3D = renderer3D;
        ID3D11Device* device = context->GetDevice();

        if (!m_shader.Load(device, gridShaderPath, "VS_Main", "PS_Main"))
            return false;

        D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0,  0,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        HRESULT hr = device->CreateInputLayout(
            layoutDesc, 2,
            m_shader.GetVSBlob()->GetBufferPointer(),
            m_shader.GetVSBlob()->GetBufferSize(),
            m_inputLayout.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Grid: CreateInputLayout failed."); return false; }

        std::vector<GridVertex> vertices;
        float extent = size * spacing;

        XMFLOAT4 normalColor = { 0.30f, 0.30f, 0.30f, 1.0f };
        XMFLOAT4 axisX = { 0.80f, 0.15f, 0.15f, 1.0f }; // red
        XMFLOAT4 axisY = { 0.15f, 0.80f, 0.15f, 1.0f }; // green
        XMFLOAT4 axisZ = { 0.15f, 0.35f, 0.80f, 1.0f }; // blue

        // Grid lines
        for (int i = -size; i <= size; i++)
        {
            float pos = i * spacing;

            XMFLOAT4 colorX = (i == 0) ? axisX : normalColor;
            vertices.push_back({ { pos, 0, -extent }, colorX });
            vertices.push_back({ { pos, 0,  extent }, colorX });

            XMFLOAT4 colorZ = (i == 0) ? axisZ : normalColor;
            vertices.push_back({ { -extent, 0, pos }, colorZ });
            vertices.push_back({ {  extent, 0, pos }, colorZ });
        }

        // Y axis — vertical line at origin
        vertices.push_back({ { 0, -extent, 0 }, axisY });
        vertices.push_back({ { 0,  extent, 0 }, axisY });

        m_vertexCount = static_cast<int>(vertices.size());

        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(GridVertex) * vertices.size());
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData{};
        vbData.pSysMem = vertices.data();

        hr = device->CreateBuffer(&vbDesc, &vbData, m_vertexBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Grid: CreateBuffer failed."); return false; }

        D3D11_BUFFER_DESC cbDesc{};
        cbDesc.ByteWidth = sizeof(XMFLOAT4X4);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Grid: CreateBuffer (CB) failed."); return false; }

        D3D11_RASTERIZER_DESC rDesc{};
        rDesc.FillMode = D3D11_FILL_SOLID;
        rDesc.CullMode = D3D11_CULL_NONE;
        rDesc.DepthClipEnable = TRUE;

        hr = device->CreateRasterizerState(&rDesc, m_rasterizerState.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Grid: CreateRasterizerState failed."); return false; }

        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

        hr = device->CreateDepthStencilState(&dsDesc, m_depthStencilState.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Grid: CreateDepthStencilState failed."); return false; }

        // Origin sphere
        m_originSphere.CreateSphere(device, 0.08f, 12, 12);
        m_originSphere.SetPosition(0.0f, 0.0f, 0.0f);

        LOG_INFO("Grid initialized ({}x{}, spacing {}).", size * 2, size * 2, spacing);
        return true;
    }

    void Grid::Draw(const Camera3D& camera)
    {
        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        // Upload VP matrix
        XMMATRIX vp = camera.GetViewProjection();
        XMFLOAT4X4 vpData;
        XMStoreFloat4x4(&vpData, vp);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        ctx->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, &vpData, sizeof(XMFLOAT4X4));
        ctx->Unmap(m_constantBuffer.Get(), 0);

        // Draw grid lines
        m_shader.Bind(ctx);
        ctx->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

        UINT stride = sizeof(GridVertex);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        ctx->RSSetState(m_rasterizerState.Get());
        ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

        ctx->Draw(m_vertexCount, 0);

        // Draw origin sphere
        if (m_renderer3D)
        {
            Material white;
            white.Albedo = { 1.0f, 1.0f, 1.0f };
            white.Metallic = 0.0f;
            white.Roughness = 0.3f;
            m_renderer3D->DrawMesh(m_originSphere,
                m_originSphere.GetWorldMatrix(), white);
        }
    }

    void Grid::Shutdown()
    {
        m_vertexBuffer.Reset();
        m_constantBuffer.Reset();
        m_inputLayout.Reset();
        m_rasterizerState.Reset();
        m_depthStencilState.Reset();
        LOG_INFO("Grid shut down.");
    }
}

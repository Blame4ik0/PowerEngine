#include "Renderer2D.h"
#include "Core/Logger.h"

namespace Engine
{
    bool Renderer2D::Init(RenderContext* context, const std::wstring& shaderPath)
    {
        m_context = context;
        ID3D11Device* device = context->GetDevice();

        if (!m_shader.Load(device, shaderPath, "VS_Main", "PS_Main"))
        {
            LOG_ERROR("Renderer2D: shader load failed.");
            return false;
        }

        // Input Layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        HRESULT hr = device->CreateInputLayout(layout, 2,
            m_shader.GetVSBlob()->GetBufferPointer(),
            m_shader.GetVSBlob()->GetBufferSize(),
            m_inputLayout.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateInputLayout failed."); return false; }

        // Vertex Buffer
        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.ByteWidth = sizeof(Vertex2D) * 4;
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = device->CreateBuffer(&vbDesc, nullptr, m_vertexBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Create vertex buffer failed."); return false; }

        // Index Buffer
        unsigned short indices[] = { 0,1,2, 2,3,0 };
        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.ByteWidth = sizeof(indices);
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA ibData = { indices };
        hr = device->CreateBuffer(&ibDesc, &ibData, m_indexBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Create index buffer failed."); return false; }

        // Constant Buffer
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Create constant buffer failed."); return false; }

        // Rasterizer State
        D3D11_RASTERIZER_DESC rDesc = {};
        rDesc.FillMode = D3D11_FILL_SOLID;
        rDesc.CullMode = D3D11_CULL_NONE;
        rDesc.DepthClipEnable = TRUE;
        device->CreateRasterizerState(&rDesc, m_rasterizerState.GetAddressOf());

        // Depth Stencil (off for 2D)
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = FALSE;
        dsDesc.StencilEnable = FALSE;
        device->CreateDepthStencilState(&dsDesc, m_depthStencilState.GetAddressOf());

        SetupOrthographicMatrix(context->GetWidth(), context->GetHeight());

        LOG_INFO("Renderer2D initialized.");
        return true;
    }

    void Renderer2D::SetupOrthographicMatrix(int screenW, int screenH)
    {
        using namespace DirectX;
        XMMATRIX ortho = XMMatrixOrthographicOffCenterLH(0.0f, (float)screenW, (float)screenH, 0.0f, 0.0f, 1.0f);
        XMStoreFloat4x4(&m_orthoMatrix, XMMatrixTranspose(ortho));
    }

    void Renderer2D::OnResize(int newWidth, int newHeight)
    {
        SetupOrthographicMatrix(newWidth, newHeight);
        LOG_INFO("Renderer2D: Ortho matrix updated to {}x{}", newWidth, newHeight);
    }

    void Renderer2D::DrawQuad(float x, float y, float w, float h, float r, float g, float b, float a)
    {
        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        // Update vertices
        Vertex2D vertices[4] =
        {
            { x,     y,     0.0f, r, g, b, a },
            { x + w, y,     0.0f, r, g, b, a },
            { x + w, y + h, 0.0f, r, g, b, a },
            { x,     y + h, 0.0f, r, g, b, a }
        };

        D3D11_MAPPED_SUBRESOURCE mapped{};
        ctx->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, vertices, sizeof(vertices));
        ctx->Unmap(m_vertexBuffer.Get(), 0);

        // Update ortho matrix
        D3D11_MAPPED_SUBRESOURCE cbMapped{};
        ctx->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbMapped);
        memcpy(cbMapped.pData, &m_orthoMatrix, sizeof(m_orthoMatrix));
        ctx->Unmap(m_constantBuffer.Get(), 0);

        m_shader.Bind(ctx);

        UINT stride = sizeof(Vertex2D);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
        ctx->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ctx->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
        ctx->RSSetState(m_rasterizerState.Get());
        ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

        ctx->DrawIndexed(6, 0, 0);
    }

    void Renderer2D::Shutdown()
    {
        m_vertexBuffer.Reset();
        m_indexBuffer.Reset();
        m_inputLayout.Reset();
        m_constantBuffer.Reset();
        m_rasterizerState.Reset();
        m_depthStencilState.Reset();
        LOG_INFO("Renderer2D shut down.");
    }
}
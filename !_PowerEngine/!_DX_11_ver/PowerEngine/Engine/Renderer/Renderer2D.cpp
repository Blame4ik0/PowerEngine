#include "Renderer2D.h"
#include "Core/Logger.h"
#include <stdexcept>

#define DX_WARN(hr, msg) \
    if (FAILED(hr)) LOG_ERROR("DrawQuad DX error at {}: HRESULT {:#x}", msg, (unsigned)hr)

using namespace DirectX;

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

        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0,  0,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        HRESULT hr = device->CreateInputLayout(
            layout, 2,
            m_shader.GetVSBlob()->GetBufferPointer(),
            m_shader.GetVSBlob()->GetBufferSize(),
            m_inputLayout.GetAddressOf()
        );
        if (FAILED(hr)) { LOG_ERROR("CreateInputLayout failed."); return false; }

        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = sizeof(Vertex2D) * 4;
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = device->CreateBuffer(&vbDesc, nullptr, m_vertexBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateBuffer (vertex) failed."); return false; }

        unsigned short indices[] = { 0, 1, 2, 2, 3, 0 };

        D3D11_BUFFER_DESC ibDesc{};
        ibDesc.ByteWidth = sizeof(indices);
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA ibData{};
        ibData.pSysMem = indices;

        hr = device->CreateBuffer(&ibDesc, &ibData, m_indexBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateBuffer (index) failed."); return false; }

        D3D11_BUFFER_DESC cbDesc{};
        cbDesc.ByteWidth = sizeof(XMFLOAT4X4);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateBuffer (constant) failed."); return false; }

        D3D11_RASTERIZER_DESC rDesc{};
        rDesc.FillMode = D3D11_FILL_SOLID;
        rDesc.CullMode = D3D11_CULL_NONE;
        rDesc.DepthClipEnable = TRUE;

        hr = device->CreateRasterizerState(&rDesc, m_rasterizerState.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateRasterizerState failed."); return false; }

        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        dsDesc.DepthEnable = FALSE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

        hr = device->CreateDepthStencilState(&dsDesc, m_depthStencilState.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateDepthStencilState failed."); return false; }

        SetupOrthographicMatrix(
            context->GetWidth(),
            context->GetHeight()
        );

        LOG_INFO("Renderer2D initialized.");
        return true;
    }

    void Renderer2D::SetupOrthographicMatrix(int screenW, int screenH)
    {
        XMMATRIX ortho = XMMatrixOrthographicOffCenterLH(
            0.0f, static_cast<float>(screenW),
            static_cast<float>(screenH), 0.0f,
            0.0f, 1.0f
        );
        XMStoreFloat4x4(&m_orthoMatrix, XMMatrixTranspose(ortho));
    }

    void Renderer2D::DrawQuad(float x, float y, float w, float h,
        float r, float g, float b, float a)
    {
        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        LOG_INFO("DrawQuad: x={} y={} w={} h={} orthoW={} orthoH={}",
            x, y, w, h, m_context->GetWidth(), m_context->GetHeight());

        Vertex2D vertices[4] =
        {
            { x,     y,     0.0f,  r, g, b, a },
            { x + w, y,     0.0f,  r, g, b, a },
            { x + w, y + h, 0.0f,  r, g, b, a },
            { x,     y + h, 0.0f,  r, g, b, a },
        };

        LOG_INFO("Vertices: TL({},{}) TR({},{}) BR({},{}) BL({},{})",
            vertices[0].x, vertices[0].y,
            vertices[1].x, vertices[1].y,
            vertices[2].x, vertices[2].y,
            vertices[3].x, vertices[3].y);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        HRESULT hr = ctx->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        DX_WARN(hr, "Map vertexBuffer");
        memcpy(mapped.pData, vertices, sizeof(vertices));
        ctx->Unmap(m_vertexBuffer.Get(), 0);

        D3D11_MAPPED_SUBRESOURCE cbMapped{};
        hr = ctx->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbMapped);
        DX_WARN(hr, "Map constantBuffer");

        // Log the ortho matrix diagonal to verify it's not zero/garbage
        LOG_INFO("OrthoMatrix[0][0]={:.4f} [1][1]={:.4f} [2][2]={:.4f} [3][3]={:.4f}",
            m_orthoMatrix._11, m_orthoMatrix._22, m_orthoMatrix._33, m_orthoMatrix._44);

        memcpy(cbMapped.pData, &m_orthoMatrix, sizeof(DirectX::XMFLOAT4X4));
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
        LOG_INFO("DrawIndexed called.");
    }

    void Renderer2D::Flush()
    {
        // no-op for now — batching comes in Phase 3.3
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

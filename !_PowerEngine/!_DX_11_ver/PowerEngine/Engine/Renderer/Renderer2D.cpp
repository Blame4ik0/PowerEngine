#include "Renderer2D.h"
#include "Core/Logger.h"

namespace Engine
{
    // Three vertices in clip space — no matrix needed
    // Clip space: center=(0,0), top=(0,1), right=(1,-1), left=(-1,-1)
    struct Vertex
    {
        float x, y;
    };

    bool Renderer2D::Init(RenderContext* context, const std::wstring& shaderPath)
    {
        m_context = context;
        ID3D11Device* device = context->GetDevice();

        // ---- Load shader ----
        if (!m_shader.Load(device, shaderPath, "VS_Main", "PS_Main"))
            return false;

        // ---- Input layout ----
        // Matches float2 Position : POSITION in the shader
        D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
              D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        HRESULT hr = device->CreateInputLayout(
            layoutDesc, 1,
            m_shader.GetVSBlob()->GetBufferPointer(),
            m_shader.GetVSBlob()->GetBufferSize(),
            m_inputLayout.GetAddressOf());

        if (FAILED(hr)) { LOG_ERROR("CreateInputLayout failed."); return false; }

        // ---- Vertex buffer — one triangle in clip space ----
        Vertex vertices[3] =
        {
            {  0.0f,  0.5f },   // top center
            {  0.5f, -0.5f },   // bottom right
            { -0.5f, -0.5f },   // bottom left
        };

        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = sizeof(vertices);
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData{};
        vbData.pSysMem = vertices;

        hr = device->CreateBuffer(&vbDesc, &vbData, m_vertexBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateBuffer (vertex) failed."); return false; }

        // ---- Rasterizer state — no culling ----
        D3D11_RASTERIZER_DESC rDesc{};
        rDesc.FillMode = D3D11_FILL_SOLID;
        rDesc.CullMode = D3D11_CULL_NONE;
        rDesc.DepthClipEnable = TRUE;

        hr = device->CreateRasterizerState(&rDesc, m_rasterizerState.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateRasterizerState failed."); return false; }

        // ---- Depth stencil — disabled for 2D ----
        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        dsDesc.DepthEnable = FALSE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

        hr = device->CreateDepthStencilState(&dsDesc, m_depthStencilState.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateDepthStencilState failed."); return false; }

        LOG_INFO("Renderer2D initialized.");
        return true;
    }

    void Renderer2D::DrawTriangle()
    {
        //LOG_INFO("DrawTriangle called");

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        m_shader.Bind(ctx);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->RSSetState(m_rasterizerState.Get());
        ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

        ctx->Draw(3, 0);
    }

    void Renderer2D::Shutdown()
    {
        m_vertexBuffer.Reset();
        m_inputLayout.Reset();
        m_rasterizerState.Reset();
        m_depthStencilState.Reset();
        LOG_INFO("Renderer2D shut down.");
    }
}

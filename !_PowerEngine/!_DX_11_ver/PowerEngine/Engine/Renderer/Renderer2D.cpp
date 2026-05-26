#include "Renderer2D.h"
#include "Core/Logger.h"

using namespace DirectX;

namespace Engine
{
    // ---- White 1x1 texture creation ----
    static bool CreateWhiteTexture(ID3D11Device* device, Texture2D& texture)
    {
        // Single white RGBA pixel
        unsigned char whitePixel[4] = { 255, 255, 255, 255 };

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = 1;
        desc.Height = 1;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = whitePixel;
        initData.SysMemPitch = 4;

        ComPtr<ID3D11Texture2D> tex;
        HRESULT hr = device->CreateTexture2D(&desc, &initData, tex.GetAddressOf());
        if (FAILED(hr)) return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        ComPtr<ID3D11ShaderResourceView> srv;
        hr = device->CreateShaderResourceView(tex.Get(), &srvDesc, srv.GetAddressOf());
        if (FAILED(hr)) return false;

        // Manually set the internals via a helper
        // We expose a friend or just load via a raw path —
        // instead we create a tiny helper directly on Texture2D
        // See note below — for now we store srv directly
        return true;
    }

    bool Renderer2D::Init(RenderContext* context, const std::wstring& shaderPath)
    {
        m_context = context;
        ID3D11Device* device = context->GetDevice();

        if (!m_shader.Load(device, shaderPath, "VS_Main", "PS_Main"))
            return false;

        // ---- Input layout — matches updated Vertex2D ----
        D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0,  0,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0,  8,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,        0, 24,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        HRESULT hr = device->CreateInputLayout(
            layoutDesc, 3,
            m_shader.GetVSBlob()->GetBufferPointer(),
            m_shader.GetVSBlob()->GetBufferSize(),
            m_inputLayout.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateInputLayout failed."); return false; }

        // ---- Vertex buffer ----
        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = sizeof(Vertex2D) * MaxVerticesPerBatch;
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = device->CreateBuffer(&vbDesc, nullptr, m_vertexBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateBuffer (vertex) failed."); return false; }

        // ---- Constant buffer ----
        D3D11_BUFFER_DESC cbDesc{};
        cbDesc.ByteWidth = sizeof(XMFLOAT4X4);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateBuffer (constant) failed."); return false; }

        // ---- Rasterizer ----
        D3D11_RASTERIZER_DESC rDesc{};
        rDesc.FillMode = D3D11_FILL_SOLID;
        rDesc.CullMode = D3D11_CULL_NONE;
        rDesc.DepthClipEnable = TRUE;

        hr = device->CreateRasterizerState(&rDesc, m_rasterizerState.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateRasterizerState failed."); return false; }

        // ---- Depth stencil ----
        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        dsDesc.DepthEnable = FALSE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

        hr = device->CreateDepthStencilState(&dsDesc, m_depthStencilState.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateDepthStencilState failed."); return false; }

        // ---- Alpha blend state ----
        D3D11_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        hr = device->CreateBlendState(&blendDesc, m_blendState.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateBlendState failed."); return false; }

        // ---- Sampler state ----
        D3D11_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        hr = device->CreateSamplerState(&samplerDesc, m_sampler.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("CreateSamplerState failed."); return false; }

        // ---- White 1x1 texture ----
        m_whiteTexture = std::make_shared<Texture2D>();
        if (!m_whiteTexture->LoadWhite(device))
        {
            LOG_ERROR("Failed to create white texture."); return false;
        }

        LOG_INFO("Renderer2D initialized.");
        LOG_WARN("Max batch: {} vertices.", MaxVerticesPerBatch);
        return true;
    }

    void Renderer2D::BeginScene(const Camera2D& camera)
    {
        m_viewProjection = camera.GetViewProjection();
        m_vertexCount = 0;
        m_drawCalls = 0;
        m_quadCount = 0;
        m_currentTexture = nullptr;
    }

    void Renderer2D::DrawQuad(float x, float y, float w, float h,
        float r, float g, float b, float a)
    {
        DrawQuad(
            { x,     y,     r, g, b, a, 0.0f, 0.0f },
            { x + w, y,     r, g, b, a, 1.0f, 0.0f },
            { x + w, y + h, r, g, b, a, 1.0f, 1.0f },
            { x,     y + h, r, g, b, a, 0.0f, 1.0f },
            nullptr
        );
    }

    void Renderer2D::DrawQuad(Vertex2D tl, Vertex2D tr,
        Vertex2D br, Vertex2D bl,
        const Texture2D* texture)
    {
        const Texture2D* tex = texture ? texture : m_whiteTexture.get();

        // If texture changes, flush the current batch first
        if (m_currentTexture != tex && m_vertexCount > 0)
            Flush();

        m_currentTexture = tex;

        if (m_vertexCount + 6 > MaxVerticesPerBatch)
            Flush();

        Vertex2D* v = &m_vertices[m_vertexCount];
        v[0] = tl;
        v[1] = tr;
        v[2] = bl;
        v[3] = tr;
        v[4] = br;
        v[5] = bl;

        m_vertexCount += 6;
        m_quadCount++;
    }

    void Renderer2D::DrawSprite(const Texture2D& texture,
        float x, float y, float w, float h,
        float r, float g, float b, float a)
    {
        DrawQuad(
            { x,     y,     r, g, b, a, 0.0f, 0.0f },
            { x + w, y,     r, g, b, a, 1.0f, 0.0f },
            { x + w, y + h, r, g, b, a, 1.0f, 1.0f },
            { x,     y + h, r, g, b, a, 0.0f, 1.0f },
            &texture
        );
    }

    void Renderer2D::DrawPolygon(Vertex2D v0, Vertex2D v1, Vertex2D v2)
    {
        // Polygons use white texture
        if (m_currentTexture != m_whiteTexture.get() && m_vertexCount > 0)
            Flush();

        m_currentTexture = m_whiteTexture.get();

        if (m_vertexCount + 3 > MaxVerticesPerBatch)
            Flush();

        m_vertices[m_vertexCount++] = v0;
        m_vertices[m_vertexCount++] = v1;
        m_vertices[m_vertexCount++] = v2;
    }

    void Renderer2D::Flush()
    {
        if (m_vertexCount == 0) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        // Upload vertices
        D3D11_MAPPED_SUBRESOURCE mapped{};
        ctx->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, m_vertices.data(), sizeof(Vertex2D) * m_vertexCount);
        ctx->Unmap(m_vertexBuffer.Get(), 0);

        // Upload matrix
        D3D11_MAPPED_SUBRESOURCE cbMapped{};
        ctx->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbMapped);
        memcpy(cbMapped.pData, &m_viewProjection, sizeof(XMFLOAT4X4));
        ctx->Unmap(m_constantBuffer.Get(), 0);

        // Bind texture and sampler
        if (m_currentTexture)
            m_currentTexture->Bind(ctx, 0);
        ctx->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

        // Bind blend state
        float blendFactor[4] = { 0, 0, 0, 0 };
        ctx->OMSetBlendState(m_blendState.Get(), blendFactor, 0xFFFFFFFF);

        // Bind everything else
        m_shader.Bind(ctx);
        ctx->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

        UINT stride = sizeof(Vertex2D);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->RSSetState(m_rasterizerState.Get());
        ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

        ctx->Draw(m_vertexCount, 0);

        m_drawCalls++;
        m_vertexCount = 0;
        m_currentTexture = nullptr;
    }

    void Renderer2D::OnResize(int width, int height) {}

    void Renderer2D::Shutdown()
    {
        m_whiteTexture.reset();
        m_vertexBuffer.Reset();
        m_constantBuffer.Reset();
        m_inputLayout.Reset();
        m_rasterizerState.Reset();
        m_depthStencilState.Reset();
        m_blendState.Reset();
        m_sampler.Reset();
        LOG_INFO("Renderer2D shut down.");
    }
}
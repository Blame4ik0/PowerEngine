#include "Renderer3D.h"
#include "Core/Logger.h"

using namespace DirectX;

namespace Engine
{
    bool Renderer3D::Init(RenderContext* context, const std::wstring& shaderPath)
    {
        m_context = context;
        ID3D11Device* device = context->GetDevice();

        if (!m_shader.Load(device, shaderPath, "VS_Main", "PS_Main"))
            return false;

        D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        HRESULT hr = device->CreateInputLayout(
            layoutDesc, 3,
            m_shader.GetVSBlob()->GetBufferPointer(),
            m_shader.GetVSBlob()->GetBufferSize(),
            m_inputLayout.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Renderer3D: CreateInputLayout failed."); return false; }

        D3D11_BUFFER_DESC cbDesc{};
        cbDesc.ByteWidth = sizeof(XMFLOAT4X4);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Renderer3D: CreateBuffer (constant) failed."); return false; }

        D3D11_RASTERIZER_DESC rDesc{};
        rDesc.FillMode = D3D11_FILL_SOLID;
        rDesc.CullMode = D3D11_CULL_BACK;
        rDesc.FrontCounterClockwise = FALSE;
        rDesc.DepthClipEnable = TRUE;

        hr = device->CreateRasterizerState(&rDesc, m_rasterizerState.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Renderer3D: CreateRasterizerState failed."); return false; }

        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

        hr = device->CreateDepthStencilState(&dsDesc, m_depthStencilState.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Renderer3D: CreateDepthStencilState failed."); return false; }

        LOG_INFO("Renderer3D initialized.");
        return true;
    }

    void Renderer3D::BeginScene(const Camera3D& camera)
    {
        m_viewProjection = camera.GetViewProjection();
    }

    void Renderer3D::DrawMesh(const Mesh& mesh, const XMMATRIX& worldMatrix)
    {
        if (!mesh.IsLoaded()) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        XMMATRIX wvp = worldMatrix * m_viewProjection;

        XMFLOAT4X4 wvpData;
        XMStoreFloat4x4(&wvpData, wvp);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        ctx->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, &wvpData, sizeof(XMFLOAT4X4));
        ctx->Unmap(m_constantBuffer.Get(), 0);

        m_shader.Bind(ctx);
        ctx->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->RSSetState(m_rasterizerState.Get());
        ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

        mesh.Draw(ctx);
    }

    void Renderer3D::DrawMesh(const Mesh& mesh,
        float x, float y, float z,
        float rotX, float rotY, float rotZ,
        float scaleX, float scaleY, float scaleZ)
    {
        XMMATRIX world =
            XMMatrixScaling(scaleX, scaleY, scaleZ) *
            XMMatrixRotationRollPitchYaw(rotX, rotY, rotZ) *
            XMMatrixTranslation(x, y, z);

        DrawMesh(mesh, world);
    }

    void Renderer3D::OnResize(float aspectRatio)
    {
    }

    void Renderer3D::Shutdown()
    {
        m_constantBuffer.Reset();
        m_inputLayout.Reset();
        m_rasterizerState.Reset();
        m_depthStencilState.Reset();
        LOG_INFO("Renderer3D shut down.");
    }
}
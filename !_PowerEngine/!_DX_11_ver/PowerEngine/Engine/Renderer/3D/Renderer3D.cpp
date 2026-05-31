#include "Renderer3D.h"
#include "Core/Logger.h"

using namespace DirectX;

namespace Engine
{
    // GPU-side constant buffer layouts — must match HLSL exactly
    struct CBPerObject
    {
        XMFLOAT4X4 World;
        XMFLOAT4X4 WorldViewProjection;
    };

    struct CBPerFrame
    {
        XMFLOAT3 CameraPosition;
        float    _pad0;
    };

    struct CBLight
    {
        XMFLOAT3 DirLightDirection;
        float    _pad1;
        XMFLOAT3 DirLightColor;
        float    DirLightIntensity;

        XMFLOAT3 PointLightPosition[4];
        float    PointLightRadius[4];
        XMFLOAT3 PointLightColor[4];
        float    PointLightIntensity[4];
        int      PointLightCount;
        XMFLOAT3 _pad2;
    };

    struct CBMaterial
    {
        XMFLOAT3 Albedo;
        float    Metallic;
        float    Roughness;
        float    AmbientOcclusion;
        XMFLOAT2 _pad;
    };

    static ComPtr<ID3D11Buffer> CreateDynamicCB(ID3D11Device* device, UINT size)
    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = size;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        ComPtr<ID3D11Buffer> buffer;
        device->CreateBuffer(&desc, nullptr, buffer.GetAddressOf());
        return buffer;
    }

    template<typename T>
    static void UpdateCB(ID3D11DeviceContext* ctx,
        ID3D11Buffer* buffer, const T& data)
    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, &data, sizeof(T));
        ctx->Unmap(buffer, 0);
    }

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

        // Create all constant buffers
        // ByteWidth must be multiple of 16
        m_cbPerObject = CreateDynamicCB(device, sizeof(CBPerObject));
        m_cbPerFrame = CreateDynamicCB(device, sizeof(CBPerFrame));
        m_cbLight = CreateDynamicCB(device, sizeof(CBLight));
        m_cbMaterial = CreateDynamicCB(device, sizeof(CBMaterial));

        if (!m_cbPerObject || !m_cbPerFrame || !m_cbLight || !m_cbMaterial)
        {
            LOG_ERROR("Renderer3D: failed to create constant buffers.");
            return false;
        }

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

        // Default directional light
        m_dirLight.Direction = { 0.5f, -1.0f, 0.5f };
        m_dirLight.Color = { 1.0f,  1.0f, 1.0f };
        m_dirLight.Intensity = 2.0f;

        LOG_INFO("Renderer3D initialized.");
        return true;
    }

    void Renderer3D::BeginScene(const Camera3D& camera)
    {
        m_view = camera.GetViewMatrix();
        m_projection = camera.GetProjectionMatrix();
        m_cameraPosition = camera.GetPosition();

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        // Upload per-frame data
        CBPerFrame perFrame;
        perFrame.CameraPosition = m_cameraPosition;
        perFrame._pad0 = 0.0f;
        UpdateCB(ctx, m_cbPerFrame.Get(), perFrame);

        // Upload light data
        UpdateLightBuffer();

        // Bind constant buffers that don't change per object
        ctx->VSSetConstantBuffers(1, 1, m_cbPerFrame.GetAddressOf());
        ctx->PSSetConstantBuffers(1, 1, m_cbPerFrame.GetAddressOf());
        ctx->PSSetConstantBuffers(2, 1, m_cbLight.GetAddressOf());
    }

    void Renderer3D::SetDirectionalLight(const DirectionalLight& light)
    {
        m_dirLight = light;
    }

    void Renderer3D::AddPointLight(const PointLight& light)
    {
        if ((int)m_pointLights.size() < MaxPointLights)
            m_pointLights.push_back(light);
        else
            LOG_WARN("Renderer3D: max point lights ({}) reached.", MaxPointLights);
    }

    void Renderer3D::ClearPointLights()
    {
        m_pointLights.clear();
    }

    void Renderer3D::UpdateLightBuffer()
    {
        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        CBLight light{};
        light.DirLightDirection = m_dirLight.Direction;
        light.DirLightColor = m_dirLight.Color;
        light.DirLightIntensity = m_dirLight.Intensity;
        light.PointLightCount = static_cast<int>(m_pointLights.size());

        for (int i = 0; i < light.PointLightCount; i++)
        {
            light.PointLightPosition[i] = m_pointLights[i].Position;
            light.PointLightRadius[i] = m_pointLights[i].Radius;
            light.PointLightColor[i] = m_pointLights[i].Color;
            light.PointLightIntensity[i] = m_pointLights[i].Intensity;
        }

        UpdateCB(ctx, m_cbLight.Get(), light);
    }

    void Renderer3D::DrawMesh(const Mesh& mesh,
        const XMMATRIX& worldMatrix,
        const Material& material)
    {
        if (!mesh.IsLoaded()) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        // Per-object constant buffer
        CBPerObject perObject;
        XMStoreFloat4x4(&perObject.World, worldMatrix);
        XMStoreFloat4x4(&perObject.WorldViewProjection,
            worldMatrix * m_view * m_projection);
        UpdateCB(ctx, m_cbPerObject.Get(), perObject);

        // Material constant buffer
        CBMaterial mat;
        mat.Albedo = material.Albedo;
        mat.Metallic = material.Metallic;
        mat.Roughness = material.Roughness;
        mat.AmbientOcclusion = material.AmbientOcclusion;
        UpdateCB(ctx, m_cbMaterial.Get(), mat);

        // Bind all constant buffers
        ctx->VSSetConstantBuffers(0, 1, m_cbPerObject.GetAddressOf());
        ctx->PSSetConstantBuffers(0, 1, m_cbPerObject.GetAddressOf());
        ctx->PSSetConstantBuffers(3, 1, m_cbMaterial.GetAddressOf());

        // Bind pipeline state
        m_shader.Bind(ctx);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->RSSetState(m_rasterizerState.Get());
        ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

        mesh.Draw(ctx);
    }

    void Renderer3D::DrawMesh(const Mesh& mesh,
        float x, float y, float z,
        float rotX, float rotY, float rotZ,
        float scaleX, float scaleY, float scaleZ,
        const Material& material)
    {
        XMMATRIX world =
            XMMatrixScaling(scaleX, scaleY, scaleZ) *
            XMMatrixRotationRollPitchYaw(rotX, rotY, rotZ) *
            XMMatrixTranslation(x, y, z);

        DrawMesh(mesh, world, material);
    }

    void Renderer3D::OnResize(float aspectRatio) {}

    void Renderer3D::Shutdown()
    {
        m_cbPerObject.Reset();
        m_cbPerFrame.Reset();
        m_cbLight.Reset();
        m_cbMaterial.Reset();
        m_inputLayout.Reset();
        m_rasterizerState.Reset();
        m_depthStencilState.Reset();
        LOG_INFO("Renderer3D shut down.");
    }
}
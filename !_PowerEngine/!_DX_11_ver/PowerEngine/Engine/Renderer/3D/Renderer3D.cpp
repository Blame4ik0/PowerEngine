#include "Renderer3D.h"
#include "Core/Logger.h"

using namespace DirectX;

namespace Engine
{
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

        XMFLOAT4 PointLightPosition[4];
        XMFLOAT4 PointLightColor[4];
        int      PointLightCount;
        XMFLOAT3 _pad2;
    };

    struct CBMaterial
    {
        XMFLOAT3 Albedo;
        float    Metallic;
        float    Roughness;
        float    AmbientOcclusion;
        int      UseAlbedoMap;
        int      UseNormalMap;
        int      UseSpecularMap;
        int      UseGlossinessMap;
        XMFLOAT2 _pad;
    };

    struct CBShadow
    {
        XMFLOAT4X4 LightSpaceMatrix;
        float      ShadowBias;
        XMFLOAT3   _pad;
    };

    struct CBShadowPass
    {
        XMFLOAT4X4 LightSpaceMatrix;
        XMFLOAT4X4 World;
    };

    static ComPtr<ID3D11Buffer> CreateDynamicCB(ID3D11Device* device, UINT size)
    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = (size + 15) & ~15; // round up to 16 bytes
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

        m_cbPerObject = CreateDynamicCB(device, sizeof(CBPerObject));
        m_cbPerFrame = CreateDynamicCB(device, sizeof(CBPerFrame));
        m_cbLight = CreateDynamicCB(device, sizeof(CBLight));
        m_cbMaterial = CreateDynamicCB(device, sizeof(CBMaterial));
        m_cbShadow = CreateDynamicCB(device, sizeof(CBShadow));
        m_cbShadowPass = CreateDynamicCB(device, sizeof(CBShadowPass));

        if (!m_cbPerObject || !m_cbPerFrame || !m_cbLight ||
            !m_cbMaterial || !m_cbShadow || !m_cbShadowPass)
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

        D3D11_SAMPLER_DESC sampDesc{};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

        hr = device->CreateSamplerState(&sampDesc, m_sampler.GetAddressOf());
        if (FAILED(hr)) { LOG_ERROR("Renderer3D: CreateSamplerState failed."); return false; }

        m_whiteTexture = std::make_shared<Texture2D>();
        m_whiteTexture->LoadWhite(device);

        if (!m_shadowMap.Init(device, L"Shaders/Shadow.hlsl", 2048))
        {
            LOG_ERROR("Renderer3D: shadow map init failed.");
            return false;
        }

        m_dirLight.Direction = { 0.5f, -1.0f, 0.5f };
        m_dirLight.Color = { 1.0f,  1.0f, 1.0f };
        m_dirLight.Intensity = 1.0f;

        LOG_INFO("Renderer3D initialized.");
        return true;
    }

    void Renderer3D::BeginScene(const Camera3D& camera)
    {
        m_view = camera.GetViewMatrix();
        m_projection = camera.GetProjectionMatrix();
        m_cameraPosition = camera.GetPosition();

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        CBPerFrame perFrame;
        perFrame.CameraPosition = m_cameraPosition;
        perFrame._pad0 = 0.0f;
        UpdateCB(ctx, m_cbPerFrame.Get(), perFrame);

        UpdateLightBuffer();

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
            light.PointLightPosition[i] = {
                m_pointLights[i].Position.x,
                m_pointLights[i].Position.y,
                m_pointLights[i].Position.z,
                m_pointLights[i].Radius
            };
            light.PointLightColor[i] = {
                m_pointLights[i].Color.x,
                m_pointLights[i].Color.y,
                m_pointLights[i].Color.z,
                m_pointLights[i].Intensity
            };
        }

        UpdateCB(ctx, m_cbLight.Get(), light);
    }

    Texture2D* Renderer3D::GetOrLoadTexture(const std::string& path)
    {
        if (path.empty()) return m_whiteTexture.get();

        auto it = m_textureCache.find(path);
        if (it != m_textureCache.end())
            return it->second.get();

        auto tex = std::make_shared<Texture2D>();
        if (!tex->Load(m_context->GetDevice(), path))
        {
            LOG_WARN("Renderer3D: texture not found '{}', using white.", path);
            return m_whiteTexture.get();
        }

        m_textureCache[path] = tex;
        return tex.get();
    }

    void Renderer3D::BeginShadowPass()
    {
        if (!m_shadowsEnabled) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        m_shadowMap.UpdateLightSpace(m_dirLight.Direction, 20.0f);
        m_shadowMap.BeginShadowPass(ctx);
        m_shadowMap.GetShader().Bind(ctx);
        ctx->IASetInputLayout(m_inputLayout.Get());

        m_currentPass = RenderPass::Shadow;
    }

    void Renderer3D::EndShadowPass()
    {
        if (!m_shadowsEnabled) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        ID3D11RenderTargetView* mainRTV = m_context->GetMainRTV();
        ID3D11DepthStencilView* mainDSV = m_context->GetMainDSV();

        D3D11_VIEWPORT vp{};
        vp.Width = static_cast<float>(m_context->GetWidth());
        vp.Height = static_cast<float>(m_context->GetHeight());
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;

        m_shadowMap.EndShadowPass(ctx, mainRTV, mainDSV, vp);

        // Rebind render target after shadow pass
        ctx->OMSetRenderTargets(1, &mainRTV, mainDSV);

        // Upload shadow constant buffer
        CBShadow shadowData;
        XMStoreFloat4x4(&shadowData.LightSpaceMatrix,
            m_shadowMap.GetLightSpaceMatrix());
        shadowData.ShadowBias = 0.005f;
        UpdateCB(ctx, m_cbShadow.Get(), shadowData);

        ctx->VSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());
        ctx->PSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());

        m_shadowMap.BindForSampling(ctx, 5);

        m_currentPass = RenderPass::Main;
    }

    void Renderer3D::DrawMesh(const Mesh& mesh,
        const XMMATRIX& worldMatrix,
        const Material& material)
    {
        if (!mesh.IsLoaded()) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        if (m_currentPass == RenderPass::Shadow)
        {
            CBShadowPass cb;
            XMStoreFloat4x4(&cb.LightSpaceMatrix,
                m_shadowMap.GetLightSpaceMatrix());
            XMStoreFloat4x4(&cb.World, worldMatrix);
            UpdateCB(ctx, m_cbShadowPass.Get(), cb);
            ctx->VSSetConstantBuffers(0, 1, m_cbShadowPass.GetAddressOf());
            mesh.Draw(ctx);
            return;
        }

        // Main pass
        CBPerObject perObject;
        XMStoreFloat4x4(&perObject.World, worldMatrix);
        XMStoreFloat4x4(&perObject.WorldViewProjection,
            worldMatrix * m_view * m_projection);
        UpdateCB(ctx, m_cbPerObject.Get(), perObject);

        Texture2D* albedo = GetOrLoadTexture(material.AlbedoMap);
        Texture2D* normal = GetOrLoadTexture(material.NormalMap);
        Texture2D* specular = GetOrLoadTexture(material.SpecularMap);
        Texture2D* glossiness = GetOrLoadTexture(material.GlossinessMap);

        albedo->Bind(ctx, 0);
        normal->Bind(ctx, 1);
        specular->Bind(ctx, 2);
        glossiness->Bind(ctx, 3);
        ctx->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

        CBMaterial mat;
        mat.Albedo = material.Albedo;
        mat.Metallic = material.Metallic;
        mat.Roughness = material.Roughness;
        mat.AmbientOcclusion = material.AmbientOcclusion;
        mat.UseAlbedoMap = !material.AlbedoMap.empty() ? 1 : 0;
        mat.UseNormalMap = !material.NormalMap.empty() ? 1 : 0;
        mat.UseSpecularMap = !material.SpecularMap.empty() ? 1 : 0;
        mat.UseGlossinessMap = !material.GlossinessMap.empty() ? 1 : 0;
        UpdateCB(ctx, m_cbMaterial.Get(), mat);

        ctx->VSSetConstantBuffers(0, 1, m_cbPerObject.GetAddressOf());
        ctx->PSSetConstantBuffers(0, 1, m_cbPerObject.GetAddressOf());
        ctx->PSSetConstantBuffers(3, 1, m_cbMaterial.GetAddressOf());

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
        m_shadowMap.Shutdown();
        m_textureCache.clear();
        m_whiteTexture.reset();
        m_cbPerObject.Reset();
        m_cbPerFrame.Reset();
        m_cbLight.Reset();
        m_cbMaterial.Reset();
        m_cbShadow.Reset();
        m_cbShadowPass.Reset();
        m_inputLayout.Reset();
        m_rasterizerState.Reset();
        m_depthStencilState.Reset();
        m_sampler.Reset();
        LOG_INFO("Renderer3D shut down.");
    }
}
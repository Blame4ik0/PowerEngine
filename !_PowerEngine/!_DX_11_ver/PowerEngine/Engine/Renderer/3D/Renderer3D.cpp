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
        float    _pad0 = 0;
    };

    struct CBLight
    {
        XMFLOAT3 DirLightDirection;
        float    _pad1 = 0;
        XMFLOAT3 DirLightColor;
        float    DirLightIntensity = 0;
        XMFLOAT4 PointLightPosition[4];
        XMFLOAT4 PointLightColor[4];
        int      PointLightCount = 0;
        XMFLOAT3 _pad2 = {};
    };

    struct CBMaterial
    {
        XMFLOAT3 Albedo = { 1, 1, 1 };
        float    Metallic = 0;
        float    Roughness = 0.5f;
        float    AmbientOcclusion = 1;
        int      UseAlbedoMap = 0;
        int      UseNormalMap = 0;
        int      UseSpecularMap = 0;
        int      UseGlossinessMap = 0;
        XMFLOAT2 _pad = {};
    };

    struct CBShadow
    {
        XMFLOAT4X4 LightSpaceMatrix;
        float      ShadowBias = 0.001f;
        int        PCFRadius = 1;
        float      TexelSize = 1.0f / 2048.0f;
        float      _pad = 0;
    };

    struct CBShadowPass
    {
        XMFLOAT4X4 LightSpaceMatrix;
        XMFLOAT4X4 World;
    };

    struct CBPointShadowPass
    {
        XMFLOAT4X4 FaceMatrix;
        XMFLOAT3   LightPos;
        float      LightRadius;
        XMFLOAT4X4 World;
    };

    struct CBPointShadowData
    {
        XMFLOAT4 PointShadowData[4]; // xyz=pos, w=radius (-1=no shadow)
        int      PointShadowCount;
        float    PointShadowBias;
        int      PoissonSamples;
        float    _pad;
    };

    static ComPtr<ID3D11Buffer> CreateDynamicCB(ID3D11Device* device, UINT size)
    {
        UINT aligned = (size + 15) & ~15u;
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = aligned;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ComPtr<ID3D11Buffer> buf;
        if (FAILED(device->CreateBuffer(&desc, nullptr, buf.GetAddressOf())))
            LOG_ERROR("CreateDynamicCB failed. size={}", size);
        return buf;
    }

    template<typename T>
    static void UpdateCB(ID3D11DeviceContext* ctx,
        ID3D11Buffer* buffer, const T& data)
    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, &data, sizeof(T));
            ctx->Unmap(buffer, 0);
        }
    }

    bool Renderer3D::Init(RenderContext* context,
        const std::wstring& shaderPath)
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
        if (FAILED(hr))
        {
            LOG_ERROR("Renderer3D: CreateInputLayout failed.");
            return false;
        }

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

        if (FAILED(device->CreateRasterizerState(&rDesc,
            m_rasterizerState.GetAddressOf())))
        {
            LOG_ERROR("Renderer3D: CreateRasterizerState failed.");
            return false;
        }

        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

        if (FAILED(device->CreateDepthStencilState(&dsDesc,
            m_depthStencilState.GetAddressOf())))
        {
            LOG_ERROR("Renderer3D: CreateDepthStencilState failed.");
            return false;
        }

        D3D11_SAMPLER_DESC sampDesc{};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

        if (FAILED(device->CreateSamplerState(&sampDesc,
            m_sampler.GetAddressOf())))
        {
            LOG_ERROR("Renderer3D: CreateSamplerState failed.");
            return false;
        }

        m_whiteTexture = std::make_shared<Texture2D>();
        m_whiteTexture->LoadWhite(device);

        if (!m_shadowMap.Init(device, L"Shaders/Shadow.hlsl", 2048))
        {
            LOG_ERROR("Renderer3D: shadow map init failed.");
            return false;
        }

        m_cbPointShadow = CreateDynamicCB(device, sizeof(CBPointShadowData));
        m_cbPointShadowPass = CreateDynamicCB(device, sizeof(CBPointShadowPass));

        if (!m_cbPointShadow || !m_cbPointShadowPass)
        {
            LOG_ERROR("Renderer3D: failed to create point shadow CBs.");
            return false;
        }

        if (!m_pointShadowMap.Init(device, L"Shaders/PointShadow.hlsl", 4, 512))
        {
            LOG_ERROR("Renderer3D: point shadow map init failed.");
            return false;
        }

        m_dirLight.Direction = { 0.5f, -1.0f, 0.5f };
        m_dirLight.Color = { 1.0f,  1.0f, 1.0f };
        m_dirLight.Intensity = 1.0f;

        LOG_INFO("Renderer3D initialized.");
        return true;
    }

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
        m_pointShadowMap.Shutdown();
        m_cbPointShadow.Reset();
        m_cbPointShadowPass.Reset();
        LOG_INFO("Renderer3D shut down.");
    }

    void Renderer3D::BeginScene(const Camera3D& camera)
    {
        m_view = camera.GetViewMatrix();
        m_projection = camera.GetProjectionMatrix();
        m_cameraPosition = camera.GetPosition();
        m_currentPass = RenderPass::Main;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        CBPerFrame perFrame;
        perFrame.CameraPosition = m_cameraPosition;
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

    void Renderer3D::SetShadowQuality(ShadowQuality quality)
    {
        m_shadowMap.SetQuality(quality);
        m_shadowMap.Shutdown();
        m_shadowMap.Init(m_context->GetDevice(), L"Shaders/Shadow.hlsl");
        LOG_INFO("Shadow quality changed. Resolution: {}",
            m_shadowMap.GetResolution());
    }

    void Renderer3D::SetPointShadowQuality(PointShadowQuality quality)
    {
        m_pointShadowMap.SetQuality(quality);
        m_pointShadowMap.Shutdown();
        m_pointShadowMap.Init(m_context->GetDevice(),
            L"Shaders/PointShadow.hlsl",
            4, m_pointShadowMap.GetResolution());
    }

    void Renderer3D::BeginShadowPass()
    {
        if (!m_shadowsEnabled) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        m_shadowMap.UpdateLightSpace(m_dirLight.Direction, 20.0f);
        m_shadowMap.BeginShadowPass(ctx);
        m_shadowMap.GetShader().Bind(ctx);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->PSSetShader(nullptr, nullptr, 0);

        m_currentPass = RenderPass::Shadow;
    }

    void Renderer3D::EndShadowPass()
    {
        if (!m_shadowsEnabled) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        ID3D11RenderTargetView* mainRTV = m_context->GetMainRTV();
        ID3D11DepthStencilView* mainDSV = m_context->GetMainDSV();

        m_shadowMap.EndShadowPass(ctx, mainRTV, mainDSV,
            m_context->GetWidth(),
            m_context->GetHeight());

        // Rebind explicitly — critical after shadow pass
        ctx->OMSetRenderTargets(1, &mainRTV, mainDSV);

        CBShadow shadowCB;
        XMStoreFloat4x4(&shadowCB.LightSpaceMatrix,
            m_shadowMap.GetLightSpaceMatrix());
        shadowCB.ShadowBias = 0.001f;
        shadowCB.PCFRadius = m_shadowMap.GetPCFRadius();
        shadowCB.TexelSize = 1.0f / static_cast<float>(m_shadowMap.GetResolution());
        UpdateCB(ctx, m_cbShadow.Get(), shadowCB);

        ctx->VSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());
        ctx->PSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());

        m_shadowMap.BindForSampling(ctx, 5, 1);

        m_currentPass = RenderPass::Main;
    }

    void Renderer3D::BeginPointShadowPass()
    {
        if (!m_pointShadowsEnabled || m_pointLights.empty()) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        // Update light data in the point shadow map
        for (int i = 0; i < (int)m_pointLights.size() && i < 4; i++)
        {
            m_pointShadowMap.UpdateLight(i,
                m_pointLights[i].Position,
                m_pointLights[i].Radius);
        }

        m_pointShadowMap.GetShader().Bind(ctx);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->PSSetShader(nullptr, nullptr, 0);

        m_currentPass = RenderPass::Shadow;
    }

    void Renderer3D::RenderPointShadowFace(int lightIndex, int face)
    {
        if (!m_pointShadowsEnabled) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        m_pointShadowMap.BeginFace(ctx, lightIndex, face);

        // Rebind shader and layout after BeginFace
        m_pointShadowMap.GetShader().Bind(ctx);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->PSSetShader(nullptr, nullptr, 0);
    }

    void Renderer3D::EndPointShadowPass()
    {
        if (!m_pointShadowsEnabled) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        ID3D11RenderTargetView* mainRTV = m_context->GetMainRTV();
        ID3D11DepthStencilView* mainDSV = m_context->GetMainDSV();

        m_pointShadowMap.EndAllFaces(ctx, mainRTV, mainDSV,
            m_context->GetWidth(),
            m_context->GetHeight());

        ctx->OMSetRenderTargets(1, &mainRTV, mainDSV);

        // Upload point shadow data for main pass
        CBPointShadowData psd{};
        int count = std::min((int)m_pointLights.size(), 4);
        for (int i = 0; i < count; i++)
        {
            psd.PointShadowData[i] = {
                m_pointLights[i].Position.x,
                m_pointLights[i].Position.y,
                m_pointLights[i].Position.z,
                m_pointShadowsEnabled ? m_pointLights[i].Radius : -1.0f
            };
        }
        // Mark unused slots
        for (int i = count; i < 4; i++)
            psd.PointShadowData[i].w = -1.0f;

        psd.PointShadowCount = count;
        psd.PointShadowBias = 0.01f;
        psd.PoissonSamples = 16;
        UpdateCB(ctx, m_cbPointShadow.Get(), psd);

        ctx->PSSetConstantBuffers(5, 1, m_cbPointShadow.GetAddressOf());

        m_pointShadowMap.BindForSampling(ctx, 6, 2);

        m_currentPass = RenderPass::Main;
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

    Texture2D* Renderer3D::ResolveTexture(
        const std::shared_ptr<Texture2D>& direct,
        const std::string& path)
    {
        if (direct) return direct.get();
        return GetOrLoadTexture(path);
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
            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            mesh.Draw(ctx);
            return;
        }

        if (m_currentPass == RenderPass::Shadow)
        {
            if (m_cbPointShadowPass) // point shadow pass
            {
                CBPointShadowPass cb{};
                // FaceMatrix and LightData set externally per face
                // Just upload world matrix
                XMFLOAT4X4 faceMatData;
                XMStoreFloat4x4(&faceMatData,
                    m_pointShadowMap.GetFaceMatrix(
                        m_activeShadowLight, m_activeShadowFace));

                CBPointShadowPass pcb;
                XMStoreFloat4x4(&pcb.FaceMatrix,
                    m_pointShadowMap.GetFaceMatrix(
                        m_activeShadowLight, m_activeShadowFace));
                pcb.LightPos = m_pointShadowMap.GetLightPosition(m_activeShadowLight);
                pcb.LightRadius = m_pointShadowMap.GetLightRadius(m_activeShadowLight);
                XMStoreFloat4x4(&pcb.World, worldMatrix);
                UpdateCB(ctx, m_cbPointShadowPass.Get(), pcb);
                ctx->VSSetConstantBuffers(0, 1, m_cbPointShadowPass.GetAddressOf());
                ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                mesh.Draw(ctx);
                return;
            }

            // Directional shadow pass
            CBShadowPass cb;
            XMStoreFloat4x4(&cb.LightSpaceMatrix,
                m_shadowMap.GetLightSpaceMatrix());
            XMStoreFloat4x4(&cb.World, worldMatrix);
            UpdateCB(ctx, m_cbShadowPass.Get(), cb);
            ctx->VSSetConstantBuffers(0, 1, m_cbShadowPass.GetAddressOf());
            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            mesh.Draw(ctx);
            return;
        }

        // Upload per-object matrices
        CBPerObject perObject;
        XMStoreFloat4x4(&perObject.World, worldMatrix);
        XMStoreFloat4x4(&perObject.WorldViewProjection,
            worldMatrix * m_view * m_projection);
        UpdateCB(ctx, m_cbPerObject.Get(), perObject);

        // Bind textures
        Texture2D* albedo = ResolveTexture(material.AlbedoTex,
            material.AlbedoMap);
        Texture2D* normal = ResolveTexture(material.NormalTex,
            material.NormalMap);
        Texture2D* specular = ResolveTexture(material.SpecularTex,
            material.SpecularMap);
        Texture2D* glossiness = ResolveTexture(material.GlossinessTex,
            material.GlossinessMap);

        albedo->Bind(ctx, 0);
        normal->Bind(ctx, 1);
        specular->Bind(ctx, 2);
        glossiness->Bind(ctx, 3);
        ctx->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

        CBMaterial mat{};
        mat.Albedo = material.Albedo;
        mat.Metallic = material.Metallic;
        mat.Roughness = material.Roughness;
        mat.AmbientOcclusion = material.AmbientOcclusion;
        mat.UseAlbedoMap = (material.AlbedoTex || !material.AlbedoMap.empty()) ? 1 : 0;
        mat.UseNormalMap = (material.NormalTex || !material.NormalMap.empty()) ? 1 : 0;
        mat.UseSpecularMap = (material.SpecularTex || !material.SpecularMap.empty()) ? 1 : 0;
        mat.UseGlossinessMap = (material.GlossinessTex || !material.GlossinessMap.empty()) ? 1 : 0;
        UpdateCB(ctx, m_cbMaterial.Get(), mat);

        ctx->VSSetConstantBuffers(0, 1, m_cbPerObject.GetAddressOf());
        ctx->PSSetConstantBuffers(0, 1, m_cbPerObject.GetAddressOf());
        ctx->PSSetConstantBuffers(3, 1, m_cbMaterial.GetAddressOf());
        ctx->VSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());
        ctx->PSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());

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

    void Renderer3D::DrawMeshAuto(const Mesh& mesh,
        const XMMATRIX& worldMatrix)
    {
        if (!mesh.IsLoaded()) return;

        // Shadow pass — just draw all geometry
        if (m_currentPass == RenderPass::Shadow)
        {
            DrawMesh(mesh, worldMatrix);
            return;
        }

        // No embedded materials — fall back to default white material
        if (!mesh.HasEmbeddedMaterials() || mesh.GetSubMeshCount() == 0)
        {
            DrawMesh(mesh, worldMatrix);
            return;
        }

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        // Upload per-object matrices once for all submeshes
        CBPerObject perObject;
        XMStoreFloat4x4(&perObject.World, worldMatrix);
        XMStoreFloat4x4(&perObject.WorldViewProjection,
            worldMatrix * m_view * m_projection);
        UpdateCB(ctx, m_cbPerObject.Get(), perObject);

        ctx->VSSetConstantBuffers(0, 1, m_cbPerObject.GetAddressOf());
        ctx->PSSetConstantBuffers(0, 1, m_cbPerObject.GetAddressOf());
        ctx->VSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());
        ctx->PSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());

        m_shader.Bind(ctx);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->RSSetState(m_rasterizerState.Get());
        ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
        ctx->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

        int lastMatIdx = -2;

        for (int s = 0; s < mesh.GetSubMeshCount(); s++)
        {
            const SubMesh& sub = mesh.GetSubMesh(s);
            int            matIdx = sub.materialIndex;

            if (matIdx != lastMatIdx)
            {
                if (matIdx >= 0 && matIdx < mesh.GetMaterialCount())
                {
                    const MeshMaterial& mm = mesh.GetMaterial(matIdx);

                    Texture2D* albedo = mm.albedo
                        ? mm.albedo.get() : m_whiteTexture.get();
                    Texture2D* normal = mm.normal
                        ? mm.normal.get() : m_whiteTexture.get();

                    // GLTF metallic-roughness packed texture
                    // G channel = roughness, B channel = metallic
                    Texture2D* specular = m_whiteTexture.get();
                    Texture2D* glossiness = m_whiteTexture.get();

                    if (mm.isMetallicRoughness && mm.metallicRoughness)
                        specular = mm.metallicRoughness.get();
                    else
                    {
                        if (mm.specular)   specular = mm.specular.get();
                        if (mm.glossiness) glossiness = mm.glossiness.get();
                    }

                    albedo->Bind(ctx, 0);
                    normal->Bind(ctx, 1);
                    specular->Bind(ctx, 2);
                    glossiness->Bind(ctx, 3);

                    CBMaterial mat{};
                    mat.Albedo = mm.albedoFactor;
                    mat.Metallic = mm.metallicFactor;
                    mat.Roughness = mm.roughnessFactor;
                    mat.AmbientOcclusion = 1.0f;
                    mat.UseAlbedoMap = mm.albedo ? 1 : 0;
                    mat.UseNormalMap = mm.normal ? 1 : 0;
                    mat.UseSpecularMap = (mm.isMetallicRoughness
                        && mm.metallicRoughness)
                        || mm.specular ? 1 : 0;
                    mat.UseGlossinessMap = (!mm.isMetallicRoughness
                        && mm.glossiness) ? 1 : 0;
                    UpdateCB(ctx, m_cbMaterial.Get(), mat);
                    ctx->PSSetConstantBuffers(3, 1, m_cbMaterial.GetAddressOf());
                }
                lastMatIdx = matIdx;
            }

            mesh.DrawSubMesh(ctx, s);
        }
    }

    void Renderer3D::OnResize(float aspectRatio) {}

    void Renderer3D::DebugDrawShadowMap(Renderer2D& renderer2D)
    {
        if (!m_shadowsEnabled) return;

        float size = 256.0f;
        float x = m_context->GetWidth() - size - 20.0f;
        float y = 20.0f;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();
        m_shadowMap.BindForSampling(ctx, 0, 0);

        renderer2D.BeginScreenSpace();
        renderer2D.DrawSprite(*m_whiteTexture, x, y, size, size,
            1.0f, 1.0f, 1.0f, 0.85f);

        ID3D11ShaderResourceView* nullSRV = nullptr;
        ctx->PSSetShaderResources(0, 1, &nullSRV);
        renderer2D.Flush();
    }
}
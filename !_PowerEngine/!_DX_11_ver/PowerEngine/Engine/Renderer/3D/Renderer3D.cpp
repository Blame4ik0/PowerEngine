#include "Renderer3D.h"
#include "Core/Logger.h"

using namespace DirectX;

namespace Engine
{
    // ================================================================
    //  Internal constant-buffer layouts  (must match Mesh.hlsl exactly)
    // ================================================================

    struct CBPerObject
    {
        XMFLOAT4X4 World;
        XMFLOAT4X4 WorldViewProjection;
        XMFLOAT4X4 ViewMatrix;
        XMFLOAT4X4 ProjectionMatrix;
    };

    struct CBPerFrame
    {
        XMFLOAT3 CameraPosition;
        float    _pad0 = 0;
    };

    struct CBLight
    {
        XMFLOAT3 DirLightDirection;  float _pad1 = 0;
        XMFLOAT3 DirLightColor;      float DirLightIntensity = 0;
        XMFLOAT4 PointLightPosition[4];   // xyz=pos, w=radius
        XMFLOAT4 PointLightColor[4];      // xyz=color, w=intensity
        int      PointLightCount = 0;
        XMFLOAT3 _pad2 = {};
    };

    struct CBMaterial
    {
        XMFLOAT3 Albedo = { 1,1,1 };
        float    Metallic = 0;
        float    Roughness = 0.5f;
        float    AmbientOcclusion = 1;
        int      UseAlbedoMap = 0;
        int      UseNormalMap = 0;
        int      UseSpecularMap = 0;
        int      UseGlossinessMap = 0;
        XMFLOAT2 _pad = {};
    };

    // b4 – used in main pass
    struct CBShadow
    {
        XMFLOAT4X4 LightSpaceMatrix;
        float ShadowBias = 0.001f;
        int   PCFRadius = 1;
        float TexelSize = 1.f / 2048.f;
        float _pad = 0;
    };

    // b4 – used only during directional shadow pass
    struct CBShadowPass
    {
        XMFLOAT4X4 LightSpaceMatrix;
        XMFLOAT4X4 World;
    };

    // b0 – used only during point shadow pass (per face)
    struct CBPointShadowPass
    {
        XMFLOAT4X4 FaceViewProj;
        XMFLOAT4X4 World;
        XMFLOAT3   LightPos;
        float      LightRadius;
    };

    // b5 – used in main pass for point shadow sampling
    struct CBPointShadowData
    {
        XMFLOAT4 PointShadowData[4]; // xyz=lightPos, w=radius
        int      PointShadowCount;
        float    PointShadowBias;
        float    _pad[2];
    };

    // ================================================================
    //  Frustum
    // ================================================================

    void Frustum::Extract(const XMMATRIX& vp)
    {
        // Gribb-Hartmann: extract planes from combined VP matrix rows
        XMFLOAT4X4 m;
        XMStoreFloat4x4(&m, XMMatrixTranspose(vp));

        // left, right, top, bottom, near, far
        planes[0] = { m._41 + m._11, m._42 + m._12, m._43 + m._13, m._44 + m._14 };
        planes[1] = { m._41 - m._11, m._42 - m._12, m._43 - m._13, m._44 - m._14 };
        planes[2] = { m._41 - m._21, m._42 - m._22, m._43 - m._23, m._44 - m._24 };
        planes[3] = { m._41 + m._21, m._42 + m._22, m._43 + m._23, m._44 + m._24 };
        planes[4] = { m._31,       m._32,       m._33,       m._34 };
        planes[5] = { m._41 - m._31, m._42 - m._32, m._43 - m._33, m._44 - m._34 };

        // Normalise
        for (auto& p : planes)
        {
            float len = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
            if (len > 0) { p.x /= len; p.y /= len; p.z /= len; p.w /= len; }
        }
    }

    bool Frustum::Intersects(const AABB& aabb, const XMMATRIX& world) const
    {
        XMFLOAT3 corners[8];
        aabb.GetCorners(world, corners);

        for (auto& p : planes)
        {
            // If all 8 corners are behind this plane — fully outside
            int outside = 0;
            for (auto& c : corners)
                if (p.x * c.x + p.y * c.y + p.z * c.z + p.w < 0.f)
                    outside++;
            if (outside == 8) return false;
        }
        return true;
    }

    // ================================================================
    //  Helpers
    // ================================================================

    static ComPtr<ID3D11Buffer> CreateDynamicCB(ID3D11Device* device,
        UINT size)
    {
        UINT aligned = (size + 15u) & ~15u;
        D3D11_BUFFER_DESC d{};
        d.ByteWidth = aligned;
        d.Usage = D3D11_USAGE_DYNAMIC;
        d.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        d.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ComPtr<ID3D11Buffer> buf;
        if (FAILED(device->CreateBuffer(&d, nullptr, buf.GetAddressOf())))
            LOG_ERROR("CreateDynamicCB failed, size={}.", size);
        return buf;
    }

    template<typename T>
    static void UpdateCB(ID3D11DeviceContext* ctx,
        ID3D11Buffer* buf, const T& data)
    {
        D3D11_MAPPED_SUBRESOURCE m{};
        if (SUCCEEDED(ctx->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &m)))
        {
            memcpy(m.pData, &data, sizeof(T));
            ctx->Unmap(buf, 0);
        }
    }

    // ================================================================
    //  Init / Shutdown
    // ================================================================

    bool Renderer3D::Init(RenderContext* context,
        const std::wstring& shaderPath)
    {
        m_context = context;
        ID3D11Device* device = context->GetDevice();

        if (!m_shader.Load(device, shaderPath, "VS_Main", "PS_Main"))
            return false;

        // Input layout — must match Vertex3D exactly
        // Vertex3D: Position(12) + Normal(12) + TexCoord(8) + Tangent(12) = 44 bytes
        // Slot 1: per-instance float4x4 world matrix (4 x float4 rows)
        D3D11_INPUT_ELEMENT_DESC lay[] =
        {
            { "POSITION",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0,
              D3D11_INPUT_PER_VERTEX_DATA,   0 },
            { "NORMAL",        0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12,
              D3D11_INPUT_PER_VERTEX_DATA,   0 },
            { "TEXCOORD",      0, DXGI_FORMAT_R32G32_FLOAT,       0, 24,
              D3D11_INPUT_PER_VERTEX_DATA,   0 },
            { "TANGENT",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 32,
              D3D11_INPUT_PER_VERTEX_DATA,   0 },
            { "INSTANCEWORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  0,
              D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCEWORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16,
              D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCEWORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32,
              D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "INSTANCEWORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48,
              D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };
        if (FAILED(device->CreateInputLayout(
            lay, 8,
            m_shader.GetVSBlob()->GetBufferPointer(),
            m_shader.GetVSBlob()->GetBufferSize(),
            m_inputLayout.GetAddressOf())))
        {
            LOG_ERROR("Renderer3D: CreateInputLayout failed.");
            return false;
        }

        // Constant buffers
        m_cbPerObject = CreateDynamicCB(device, sizeof(CBPerObject));
        m_cbPerFrame = CreateDynamicCB(device, sizeof(CBPerFrame));
        m_cbLight = CreateDynamicCB(device, sizeof(CBLight));
        m_cbMaterial = CreateDynamicCB(device, sizeof(CBMaterial));
        m_cbShadow = CreateDynamicCB(device, sizeof(CBShadow));
        m_cbShadowPass = CreateDynamicCB(device, sizeof(CBShadowPass));
        m_cbPointShadow = CreateDynamicCB(device, sizeof(CBPointShadowData));
        m_cbPointShadowPass = CreateDynamicCB(device, sizeof(CBPointShadowPass));

        if (!m_cbPerObject || !m_cbPerFrame || !m_cbLight ||
            !m_cbMaterial || !m_cbShadow || !m_cbShadowPass ||
            !m_cbPointShadow || !m_cbPointShadowPass)
        {
            LOG_ERROR("Renderer3D: constant buffer creation failed.");
            return false;
        }

        // Instance buffer — holds up to MaxInstanceCount world matrices
        {
            D3D11_BUFFER_DESC d{};
            d.ByteWidth = (UINT)(sizeof(XMFLOAT4X4) * MaxInstanceCount);
            d.Usage = D3D11_USAGE_DYNAMIC;
            d.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            d.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (FAILED(device->CreateBuffer(&d, nullptr,
                m_instanceBuffer.GetAddressOf())))
            {
                LOG_ERROR("Renderer3D: instance buffer creation failed.");
                return false;
            }
        }

        // Main-pass rasterizer
        {
            D3D11_RASTERIZER_DESC d{};
            d.FillMode = D3D11_FILL_SOLID;
            d.CullMode = D3D11_CULL_BACK;
            d.DepthClipEnable = TRUE;
            device->CreateRasterizerState(&d,
                m_rasterizerState.GetAddressOf());
        }

        // Main-pass depth-stencil
        {
            D3D11_DEPTH_STENCIL_DESC d{};
            d.DepthEnable = TRUE;
            d.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            d.DepthFunc = D3D11_COMPARISON_LESS;
            device->CreateDepthStencilState(&d,
                m_depthStencilState.GetAddressOf());
        }

        // Texture sampler (wrap, anisotropic)
        {
            D3D11_SAMPLER_DESC d{};
            d.Filter = D3D11_FILTER_ANISOTROPIC;
            d.MaxAnisotropy = 16; // hardware caps to 16 max anyway
            d.AddressU = d.AddressV = d.AddressW =
                D3D11_TEXTURE_ADDRESS_WRAP;
            d.ComparisonFunc = D3D11_COMPARISON_NEVER;
            d.MaxLOD = D3D11_FLOAT32_MAX;
            device->CreateSamplerState(&d, m_sampler.GetAddressOf());
        }

        m_whiteTexture = std::make_shared<Texture2D>();
        m_whiteTexture->LoadWhite(device);

        // Directional shadow map
        if (!m_shadowMap.Init(device, L"Shaders/Shadow.hlsl", 2048))
        {
            LOG_ERROR("Renderer3D: shadow map init failed.");
            return false;
        }

        // Point shadow map
        if (!m_pointShadowMap.Init(device,
            L"Shaders/PointShadow.hlsl", 4, 512))
        {
            LOG_ERROR("Renderer3D: point shadow map init failed.");
            return false;
        }

        m_dirLight = { {0.5f,-1.f,0.5f}, 0, {1,1,1}, 1 };

        LOG_INFO("Renderer3D initialized.");
        return true;
    }

    void Renderer3D::Shutdown()
    {
        m_shadowMap.Shutdown();
        m_pointShadowMap.Shutdown();
        m_textureCache.clear();
        m_whiteTexture.reset();
        m_cbPerObject.Reset();
        m_cbPerFrame.Reset();
        m_cbLight.Reset();
        m_cbMaterial.Reset();
        m_cbShadow.Reset();
        m_cbShadowPass.Reset();
        m_cbPointShadow.Reset();
        m_cbPointShadowPass.Reset();
        m_inputLayout.Reset();
        m_rasterizerState.Reset();
        m_depthStencilState.Reset();
        m_sampler.Reset();
        LOG_INFO("Renderer3D shut down.");
    }

    // ================================================================
    //  Scene setup
    // ================================================================

    void Renderer3D::BeginScene(const Camera3D& camera)
    {
        m_view = camera.GetViewMatrix();
        m_projection = camera.GetProjectionMatrix();
        m_cameraPosition = camera.GetPosition();
        m_currentPass = RenderPass::Main;
        m_inPointShadowPass = false;
        m_culledCount = 0;

        m_frustum.Extract(m_view * m_projection);

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        CBPerFrame pf;
        pf.CameraPosition = m_cameraPosition;
        UpdateCB(ctx, m_cbPerFrame.Get(), pf);
        UpdateLightBuffer();

        ctx->VSSetConstantBuffers(1, 1, m_cbPerFrame.GetAddressOf());
        ctx->PSSetConstantBuffers(1, 1, m_cbPerFrame.GetAddressOf());
        ctx->PSSetConstantBuffers(2, 1, m_cbLight.GetAddressOf());
    }

    void Renderer3D::SetDirectionalLight(const DirectionalLight& l)
    {
        m_dirLight = l;
    }

    void Renderer3D::AddPointLight(const PointLight& l)
    {
        if ((int)m_pointLights.size() < MaxPointLights)
            m_pointLights.push_back(l);
    }

    void Renderer3D::ClearPointLights()
    {
        m_pointLights.clear();
    }

    void Renderer3D::UpdateLightBuffer()
    {
        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();
        CBLight cb{};
        cb.DirLightDirection = m_dirLight.Direction;
        cb.DirLightColor = m_dirLight.Color;
        cb.DirLightIntensity = m_dirLight.Intensity;
        cb.PointLightCount = (int)m_pointLights.size();
        for (int i = 0; i < cb.PointLightCount; i++)
        {
            auto& pl = m_pointLights[i];
            cb.PointLightPosition[i] = { pl.Position.x, pl.Position.y,
                                         pl.Position.z, pl.Radius };
            cb.PointLightColor[i] = { pl.Color.x, pl.Color.y,
                                         pl.Color.z, pl.Intensity };
        }
        UpdateCB(ctx, m_cbLight.Get(), cb);
    }

    // ================================================================
    //  Quality setters
    // ================================================================

    void Renderer3D::SetShadowQuality(ShadowQuality q)
    {
        m_shadowMap.SetQuality(q);
        m_shadowMap.Shutdown();
        m_shadowMap.Init(m_context->GetDevice(), L"Shaders/Shadow.hlsl");
        LOG_INFO("Dir shadow quality -> {}.", m_shadowMap.GetResolution());
    }

    void Renderer3D::SetPointShadowQuality(PointShadowQuality q)
    {
        m_pointShadowMap.SetQuality(q);
        m_pointShadowMap.Shutdown();
        m_pointShadowMap.Init(m_context->GetDevice(),
            L"Shaders/PointShadow.hlsl",
            4, m_pointShadowMap.GetResolution());
        LOG_INFO("Pt shadow quality -> {}.", m_pointShadowMap.GetResolution());
    }

    // ================================================================
    //  Directional shadow pass
    // ================================================================

    void Renderer3D::BeginShadowPass()
    {
        if (!m_shadowsEnabled) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();
        m_shadowMap.UpdateLightSpace(m_dirLight.Direction, 20.f);
        m_shadowMap.BeginShadowPass(ctx);
        m_shadowMap.GetShader().Bind(ctx);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->PSSetShader(nullptr, nullptr, 0);

        m_currentPass = RenderPass::Shadow;
        m_inPointShadowPass = false;
    }

    void Renderer3D::EndShadowPass()
    {
        if (!m_shadowsEnabled) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();
        auto* rtv = m_context->GetMainRTV();
        auto* dsv = m_context->GetMainDSV();
        m_shadowMap.EndShadowPass(ctx, rtv, dsv,
            m_context->GetWidth(), m_context->GetHeight());
        ctx->OMSetRenderTargets(1, &rtv, dsv);

        CBShadow sc;
        XMStoreFloat4x4(&sc.LightSpaceMatrix,
            m_shadowMap.GetLightSpaceMatrix());
        sc.ShadowBias = 0.001f;
        sc.PCFRadius = m_shadowMap.GetPCFRadius();
        sc.TexelSize = 1.f / (float)m_shadowMap.GetResolution();
        UpdateCB(ctx, m_cbShadow.Get(), sc);
        ctx->VSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());
        ctx->PSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());
        m_shadowMap.BindForSampling(ctx, 5, 1);

        m_currentPass = RenderPass::Main;
    }

    // ================================================================
    //  Point shadow pass
    // ================================================================

    void Renderer3D::BeginPointShadowPass()
    {
        if (!m_pointShadowsEnabled || m_pointLights.empty()) return;

        // Push light positions/radii into the shadow map object
        int n = std::min((int)m_pointLights.size(), 4);
        for (int i = 0; i < n; i++)
            m_pointShadowMap.UpdateLight(i,
                m_pointLights[i].Position,
                m_pointLights[i].Radius);

        m_currentPass = RenderPass::Shadow;
        m_inPointShadowPass = true;
    }

    void Renderer3D::RenderPointShadowFace(int lightIndex, int face)
    {
        if (!m_pointShadowsEnabled) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        m_activeShadowLight = lightIndex;
        m_activeShadowFace = face;

        // BeginFace: clears DSV, binds null RTV + DSV, sets viewport/states
        m_pointShadowMap.BeginFace(ctx, lightIndex, face);

        // Bind point-shadow VS (no PS — depth only, exactly like dir shadows)
        m_pointShadowMap.GetShader().Bind(ctx);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->PSSetShader(nullptr, nullptr, 0);
    }

    void Renderer3D::EndPointShadowPass()
    {
        if (!m_pointShadowsEnabled) return;

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();
        auto* rtv = m_context->GetMainRTV();
        auto* dsv = m_context->GetMainDSV();

        // Restore main render target + viewport
        m_pointShadowMap.EndAllFaces(ctx, rtv, dsv,
            m_context->GetWidth(), m_context->GetHeight());
        ctx->OMSetRenderTargets(1, &rtv, dsv);

        // Upload point shadow CB for sampling in main pass
        CBPointShadowData psd{};
        int n = std::min((int)m_pointLights.size(), 4);
        for (int i = 0; i < n; i++)
        {
            psd.PointShadowData[i] = {
                m_pointLights[i].Position.x,
                m_pointLights[i].Position.y,
                m_pointLights[i].Position.z,
                m_pointLights[i].Radius
            };
        }
        for (int i = n; i < 4; i++) psd.PointShadowData[i].w = -1.f;
        psd.PointShadowCount = n;
        psd.PointShadowBias = 0.005f;   // small bias — hardware depth is precise
        UpdateCB(ctx, m_cbPointShadow.Get(), psd);
        ctx->PSSetConstantBuffers(5, 1, m_cbPointShadow.GetAddressOf());
        m_pointShadowMap.BindForSampling(ctx, 6, 2);

        m_currentPass = RenderPass::Main;
        m_inPointShadowPass = false;
    }

    // ================================================================
    //  DrawMesh
    // ================================================================

    void Renderer3D::DrawMesh(const Mesh& mesh,
        const XMMATRIX& worldMatrix,
        const Material& material)
    {
        if (!mesh.IsLoaded()) return;
        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        // ---- shadow passes ----
        if (m_currentPass == RenderPass::Shadow)
        {
            if (m_inPointShadowPass)
            {
                // Rebind shadow shader + PS every draw call
                m_pointShadowMap.GetShader().Bind(ctx);
                ctx->IASetInputLayout(m_inputLayout.Get());

                // Point shadow face pass
                CBPointShadowPass pcb{};
                XMStoreFloat4x4(&pcb.FaceViewProj,
                    m_pointShadowMap.GetFaceMatrix(
                        m_activeShadowLight, m_activeShadowFace));
                XMStoreFloat4x4(&pcb.World, worldMatrix);
                pcb.LightPos = m_pointShadowMap.GetLightPosition(m_activeShadowLight);
                pcb.LightRadius = m_pointShadowMap.GetLightRadius(m_activeShadowLight);
                UpdateCB(ctx, m_cbPointShadowPass.Get(), pcb);
                ctx->VSSetConstantBuffers(0, 1,
                    m_cbPointShadowPass.GetAddressOf());
                ctx->PSSetConstantBuffers(0, 1,
                    m_cbPointShadowPass.GetAddressOf());
                ctx->IASetPrimitiveTopology(
                    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                mesh.Draw(ctx);
            }
            else
            {
                // Directional shadow pass
                CBShadowPass scb{};
                XMStoreFloat4x4(&scb.LightSpaceMatrix,
                    m_shadowMap.GetLightSpaceMatrix());
                XMStoreFloat4x4(&scb.World, worldMatrix);
                UpdateCB(ctx, m_cbShadowPass.Get(), scb);
                ctx->VSSetConstantBuffers(0, 1,
                    m_cbShadowPass.GetAddressOf());
                ctx->IASetPrimitiveTopology(
                    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                mesh.Draw(ctx);
            }
            return;
        }

        // ---- main pass ----

        // Frustum cull — skip if AABB is outside all 6 planes
        if (m_frustumCullEnabled &&
            !m_frustum.Intersects(mesh.GetAABB(), worldMatrix))
        {
            m_culledCount++;
            return;
        }

        CBPerObject po;
        XMStoreFloat4x4(&po.World, worldMatrix);
        XMStoreFloat4x4(&po.WorldViewProjection,
            worldMatrix * m_view * m_projection);
        XMStoreFloat4x4(&po.ViewMatrix, m_view);
        XMStoreFloat4x4(&po.ProjectionMatrix, m_projection);
        UpdateCB(ctx, m_cbPerObject.Get(), po);

        auto* alb = ResolveTexture(material.AlbedoTex, material.AlbedoMap);
        auto* nrm = ResolveTexture(material.NormalTex, material.NormalMap);
        auto* spec = ResolveTexture(material.SpecularTex, material.SpecularMap);
        auto* glos = ResolveTexture(material.GlossinessTex, material.GlossinessMap);

        alb->Bind(ctx, 0);
        nrm->Bind(ctx, 1);
        spec->Bind(ctx, 2);
        glos->Bind(ctx, 3);
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
        ctx->PSSetConstantBuffers(5, 1, m_cbPointShadow.GetAddressOf());
        m_pointShadowMap.BindForSampling(ctx, 6, 2);

        m_shader.Bind(ctx);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->RSSetState(m_rasterizerState.Get());
        ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

        // Slot 1 must be bound even for non-instanced draws (instance rows = 0)
        UINT instStride = sizeof(XMFLOAT4X4);
        UINT instOffset = 0;
        ctx->IASetVertexBuffers(1, 1,
            m_instanceBuffer.GetAddressOf(), &instStride, &instOffset);

        mesh.Draw(ctx);
    }

    void Renderer3D::DrawMesh(const Mesh& mesh,
        float x, float y, float z,
        float rx, float ry, float rz,
        float sx, float sy, float sz,
        const Material& material)
    {
        XMMATRIX w = XMMatrixScaling(sx, sy, sz)
            * XMMatrixRotationRollPitchYaw(rx, ry, rz)
            * XMMatrixTranslation(x, y, z);
        DrawMesh(mesh, w, material);
    }

    void Renderer3D::DrawMeshAuto(const Mesh& mesh,
        const XMMATRIX& worldMatrix)
    {
        if (!mesh.IsLoaded()) return;

        // Shadow passes — no embedded materials needed
        if (m_currentPass == RenderPass::Shadow)
        {
            DrawMesh(mesh, worldMatrix);
            return;
        }

        if (!mesh.HasEmbeddedMaterials() || mesh.GetSubMeshCount() == 0)
        {
            DrawMesh(mesh, worldMatrix);
            return;
        }

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        CBPerObject po;
        XMStoreFloat4x4(&po.World, worldMatrix);
        XMStoreFloat4x4(&po.WorldViewProjection,
            worldMatrix * m_view * m_projection);
        XMStoreFloat4x4(&po.ViewMatrix, m_view);
        XMStoreFloat4x4(&po.ProjectionMatrix, m_projection);
        UpdateCB(ctx, m_cbPerObject.Get(), po);

        ctx->VSSetConstantBuffers(0, 1, m_cbPerObject.GetAddressOf());
        ctx->PSSetConstantBuffers(0, 1, m_cbPerObject.GetAddressOf());
        ctx->VSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());
        ctx->PSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());
        ctx->PSSetConstantBuffers(5, 1, m_cbPointShadow.GetAddressOf());
        m_pointShadowMap.BindForSampling(ctx, 6, 2);

        m_shader.Bind(ctx);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->RSSetState(m_rasterizerState.Get());
        ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
        ctx->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

        // Slot 1 must be bound even for non-instanced draws
        UINT instStride = sizeof(XMFLOAT4X4);
        UINT instOffset = 0;
        ctx->IASetVertexBuffers(1, 1,
            m_instanceBuffer.GetAddressOf(), &instStride, &instOffset);

        int lastMat = -2;
        for (int s = 0; s < mesh.GetSubMeshCount(); s++)
        {
            int matIdx = mesh.GetSubMesh(s).materialIndex;
            if (matIdx != lastMat)
            {
                if (matIdx >= 0 && matIdx < mesh.GetMaterialCount())
                {
                    const MeshMaterial& mm = mesh.GetMaterial(matIdx);
                    (mm.albedo ? mm.albedo.get() : m_whiteTexture.get())
                        ->Bind(ctx, 0);
                    (mm.normal ? mm.normal.get() : m_whiteTexture.get())
                        ->Bind(ctx, 1);

                    Texture2D* spec = m_whiteTexture.get();
                    Texture2D* glos = m_whiteTexture.get();
                    if (mm.isMetallicRoughness && mm.metallicRoughness)
                        spec = mm.metallicRoughness.get();
                    else {
                        if (mm.specular)   spec = mm.specular.get();
                        if (mm.glossiness) glos = mm.glossiness.get();
                    }
                    spec->Bind(ctx, 2);
                    glos->Bind(ctx, 3);

                    CBMaterial mat{};
                    mat.Albedo = mm.albedoFactor;
                    mat.Metallic = mm.metallicFactor;
                    mat.Roughness = mm.roughnessFactor;
                    mat.AmbientOcclusion = 1.f;
                    mat.UseAlbedoMap = mm.albedo ? 1 : 0;
                    mat.UseNormalMap = mm.normal ? 1 : 0;
                    mat.UseSpecularMap = (mm.isMetallicRoughness &&
                        mm.metallicRoughness)
                        || mm.specular ? 1 : 0;
                    mat.UseGlossinessMap = (!mm.isMetallicRoughness &&
                        mm.glossiness) ? 1 : 0;

                    // Apply material overrides (per-material wins over global)
                    const MaterialOverride* ov = mesh.ResolveOverride(matIdx);
                    if (ov)
                    {
                        if (ov->overrideAlbedo)    mat.Albedo = ov->albedo;
                        if (ov->overrideMetallic)  mat.Metallic = ov->metallic;
                        if (ov->overrideRoughness) mat.Roughness = ov->roughness;
                    }

                    UpdateCB(ctx, m_cbMaterial.Get(), mat);
                    ctx->PSSetConstantBuffers(3, 1,
                        m_cbMaterial.GetAddressOf());
                }
                lastMat = matIdx;
            }
            mesh.DrawSubMesh(ctx, s);
        }
    }

    void Renderer3D::DrawMeshInstanced(
        const Mesh& mesh,
        const Material& material,
        const std::vector<XMMATRIX>& transforms)
    {
        if (!mesh.IsLoaded() || transforms.empty()) return;
        if (m_currentPass == RenderPass::Shadow)    return;

        int count = std::min((int)transforms.size(), MaxInstanceCount);

        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();

        // Upload instance world matrices into instance buffer
        {
            D3D11_MAPPED_SUBRESOURCE ms{};
            ctx->Map(m_instanceBuffer.Get(), 0,
                D3D11_MAP_WRITE_DISCARD, 0, &ms);
            auto* dst = reinterpret_cast<XMFLOAT4X4*>(ms.pData);
            for (int i = 0; i < count; i++)
                XMStoreFloat4x4(&dst[i],
                    XMMatrixTranspose(transforms[i]));
            ctx->Unmap(m_instanceBuffer.Get(), 0);
        }

        // Bind vertex + instance buffers to slots 0 and 1
        // (mesh's own VB is slot 0, instance buffer is slot 1)
        UINT strides[2] = { sizeof(Vertex3D), sizeof(XMFLOAT4X4) };
        UINT offsets[2] = { 0, 0 };
        ID3D11Buffer* vbs[2] = { nullptr, m_instanceBuffer.Get() };
        // Get mesh VB via Draw internals — draw normally for single,
        // for instanced we call DrawIndexedInstanced directly after binding.
        // Bind material
        auto* alb = ResolveTexture(material.AlbedoTex, material.AlbedoMap);
        auto* nrm = ResolveTexture(material.NormalTex, material.NormalMap);
        auto* spec = ResolveTexture(material.SpecularTex, material.SpecularMap);
        auto* glos = ResolveTexture(material.GlossinessTex, material.GlossinessMap);

        alb->Bind(ctx, 0);
        nrm->Bind(ctx, 1);
        spec->Bind(ctx, 2);
        glos->Bind(ctx, 3);
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

        ctx->PSSetConstantBuffers(3, 1, m_cbMaterial.GetAddressOf());
        ctx->PSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());
        ctx->PSSetConstantBuffers(5, 1, m_cbPointShadow.GetAddressOf());
        ctx->VSSetConstantBuffers(4, 1, m_cbShadow.GetAddressOf());
        m_pointShadowMap.BindForSampling(ctx, 6, 2);

        m_shader.Bind(ctx);
        ctx->IASetInputLayout(m_inputLayout.Get());
        ctx->RSSetState(m_rasterizerState.Get());
        ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Bind mesh buffers to slot 0, instance buffer to slot 1
        mesh.BindBuffers(ctx, m_instanceBuffer.Get(), count);
    }

    void Renderer3D::DrawLodGroup(
        const LodGroup& group,
        const XMMATRIX& worldMatrix,
        const Material& material)
    {
        if (group.Empty()) return;

        // World position is the translation row of the world matrix
        XMVECTOR worldPos = worldMatrix.r[3];
        XMVECTOR camPos = XMLoadFloat3(&m_cameraPosition);
        float distance = XMVectorGetX(XMVector3Length(worldPos - camPos));

        Mesh* selected = group.Select(distance);
        if (!selected) return;

        DrawMesh(*selected, worldMatrix, material);
    }

    void Renderer3D::OnResize(float) {}

    // ================================================================
    //  Texture helpers
    // ================================================================

    Texture2D* Renderer3D::GetOrLoadTexture(const std::string& path)
    {
        if (path.empty()) return m_whiteTexture.get();
        auto it = m_textureCache.find(path);
        if (it != m_textureCache.end()) return it->second.get();
        auto tex = std::make_shared<Texture2D>();
        if (!tex->Load(m_context->GetDevice(), m_context->GetDeviceContext(), path))
        {
            LOG_WARN("Renderer3D: '{}' not found, using white.", path);
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

    void Renderer3D::DebugDrawShadowMap(Renderer2D& renderer2D)
    {
        if (!m_shadowsEnabled) return;
        float size = 256.f;
        float x = m_context->GetWidth() - size - 20.f;
        float y = 20.f;
        ID3D11DeviceContext* ctx = m_context->GetDeviceContext();
        m_shadowMap.BindForSampling(ctx, 0, 0);
        renderer2D.BeginScreenSpace();
        renderer2D.DrawSprite(*m_whiteTexture, x, y, size, size,
            1.f, 1.f, 1.f, 0.85f);
        ID3D11ShaderResourceView* n = nullptr;
        ctx->PSSetShaderResources(0, 1, &n);
        renderer2D.Flush();
    }

} // namespace Engine
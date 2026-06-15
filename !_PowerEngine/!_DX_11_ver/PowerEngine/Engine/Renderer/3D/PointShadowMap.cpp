#include "PointShadowMap.h"
#include "Core/Logger.h"

using namespace DirectX;

namespace Engine
{
    static const XMFLOAT3 s_targets[6] =
    {
        {  1.0f,  0.0f,  0.0f }, // +X
        { -1.0f,  0.0f,  0.0f }, // -X
        {  0.0f,  1.0f,  0.0f }, // +Y
        {  0.0f, -1.0f,  0.0f }, // -Y
        {  0.0f,  0.0f,  1.0f }, // +Z
        {  0.0f,  0.0f, -1.0f }, // -Z
    };

    static const XMFLOAT3 s_ups[6] =
    {
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f },
        { 0.0f, 0.0f,  1.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
    };

    void PointShadowMap::SetQuality(PointShadowQuality quality)
    {
        m_quality = quality;
        switch (quality)
        {
        case PointShadowQuality::Low:    m_resolution = 256;  break;
        case PointShadowQuality::Medium: m_resolution = 512;  break;
        case PointShadowQuality::High:   m_resolution = 1024; break;
        case PointShadowQuality::Ultra:  m_resolution = 2048; break;
        default:                         m_resolution = 512;  break;
        }
    }

    bool PointShadowMap::Init(ID3D11Device* device,
        const std::wstring& shaderPath,
        int maxLights,
        int faceResolution)
    {
        if (!device || maxLights <= 0 || faceResolution <= 0)
            return false;

        m_maxLights = maxLights;
        m_resolution = faceResolution;

        if (!m_shader.Load(device, shaderPath, "VS_Main", "PS_Main"))
            return false;

        m_lights.resize(maxLights);
        m_faceMatrices.resize(maxLights * 6, XMMatrixIdentity());

        if (!CreateTextures(device))
            return false;

        // Linear sampler (no comparison)
        D3D11_SAMPLER_DESC sd{};
        sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sd.MinLOD = 0;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(device->CreateSamplerState(&sd, m_sampler.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateSamplerState failed.");
            return false;
        }

        // Depth-stencil state
        D3D11_DEPTH_STENCIL_DESC dd{};
        dd.DepthEnable = TRUE;
        dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dd.DepthFunc = D3D11_COMPARISON_LESS;
        dd.StencilEnable = FALSE;
        if (FAILED(device->CreateDepthStencilState(&dd, m_depthStencilState.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateDepthStencilState failed.");
            return false;
        }

        // Rasterizer state
        D3D11_RASTERIZER_DESC rd{};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_BACK;
        rd.FrontCounterClockwise = FALSE;
        rd.DepthClipEnable = TRUE;
        if (FAILED(device->CreateRasterizerState(&rd, m_rasterizerState.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateRasterizerState failed.");
            return false;
        }

        m_viewport = { 0.0f, 0.0f,
                       static_cast<float>(m_resolution),
                       static_cast<float>(m_resolution),
                       0.0f, 1.0f };

        LOG_INFO("PointShadowMap initialized ({} lights, {}x{} per face).", maxLights, m_resolution, m_resolution);
        return true;
    }

    bool PointShadowMap::CreateTextures(ID3D11Device* device)
    {
        m_colorTex.Reset();
        m_srv.Reset();
        m_rtvs.clear();
        m_depthTex.Reset();
        m_depthDSV.Reset();

        // Color texture: Cube Array (R32_FLOAT linear depth)
        D3D11_TEXTURE2D_DESC cd{};
        cd.Width = cd.Height = static_cast<UINT>(m_resolution);
        cd.MipLevels = 1;
        cd.ArraySize = static_cast<UINT>(m_maxLights * 6);
        cd.Format = DXGI_FORMAT_R32_FLOAT;
        cd.SampleDesc = { 1, 0 };
        cd.Usage = D3D11_USAGE_DEFAULT;
        cd.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        cd.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        if (FAILED(device->CreateTexture2D(&cd, nullptr, m_colorTex.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateTexture2D (color) failed.");
            return false;
        }

        // Create one RTV per face per light
        m_rtvs.resize(m_maxLights * 6);
        for (int l = 0; l < m_maxLights; ++l)
        {
            for (int f = 0; f < 6; ++f)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rv{};
                rv.Format = DXGI_FORMAT_R32_FLOAT;
                rv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                rv.Texture2DArray.MipSlice = 0;
                rv.Texture2DArray.FirstArraySlice = l * 6 + f;
                rv.Texture2DArray.ArraySize = 1;

                if (FAILED(device->CreateRenderTargetView(m_colorTex.Get(), &rv,
                    m_rtvs[l * 6 + f].GetAddressOf())))
                {
                    LOG_ERROR("PointShadowMap: Failed to create RTV for light {} face {}", l, f);
                    return false;
                }
            }
        }

        // SRV as TextureCubeArray
        D3D11_SHADER_RESOURCE_VIEW_DESC sv{};
        sv.Format = DXGI_FORMAT_R32_FLOAT;
        sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
        sv.TextureCubeArray.MostDetailedMip = 0;
        sv.TextureCubeArray.MipLevels = 1;
        sv.TextureCubeArray.First2DArrayFace = 0;
        sv.TextureCubeArray.NumCubes = m_maxLights;

        if (FAILED(device->CreateShaderResourceView(m_colorTex.Get(), &sv, m_srv.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateShaderResourceView failed.");
            return false;
        }

        // Shared depth buffer
        D3D11_TEXTURE2D_DESC dd{};
        dd.Width = dd.Height = static_cast<UINT>(m_resolution);
        dd.MipLevels = 1;
        dd.ArraySize = 1;
        dd.Format = DXGI_FORMAT_D32_FLOAT;
        dd.SampleDesc = { 1, 0 };
        dd.Usage = D3D11_USAGE_DEFAULT;
        dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        if (FAILED(device->CreateTexture2D(&dd, nullptr, m_depthTex.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateTexture2D (depth) failed.");
            return false;
        }

        if (FAILED(device->CreateDepthStencilView(m_depthTex.Get(), nullptr, m_depthDSV.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateDepthStencilView failed.");
            return false;
        }

        return true;
    }

    void PointShadowMap::UpdateLight(int lightIndex, const XMFLOAT3& position, float radius)
    {
        if (lightIndex < 0 || lightIndex >= m_maxLights) return;

        m_lights[lightIndex].position = position;
        m_lights[lightIndex].radius = radius;

        XMVECTOR pos = XMLoadFloat3(&position);
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, 0.05f, radius);

        for (int f = 0; f < 6; ++f)
        {
            XMVECTOR tgt = pos + XMLoadFloat3(&s_targets[f]);
            XMVECTOR up = XMLoadFloat3(&s_ups[f]);

            m_faceMatrices[lightIndex * 6 + f] = XMMatrixLookAtLH(pos, tgt, up) * proj;
        }
    }

    void PointShadowMap::BeginFace(ID3D11DeviceContext* ctx, int lightIndex, int face)
    {
        if (!ctx || lightIndex < 0 || lightIndex >= m_maxLights || face < 0 || face >= 6)
            return;

        // Unbind shadow map from pixel shader to avoid read/write conflict
        ID3D11ShaderResourceView* nullSRV = nullptr;
        ctx->PSSetShaderResources(6, 1, &nullSRV);

        int idx = lightIndex * 6 + face;

        float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        ctx->ClearRenderTargetView(m_rtvs[idx].Get(), clearColor);
        ctx->ClearDepthStencilView(m_depthDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

        ctx->OMSetRenderTargets(1, m_rtvs[idx].GetAddressOf(), m_depthDSV.Get());
        ctx->RSSetViewports(1, &m_viewport);
        ctx->RSSetState(m_rasterizerState.Get());
        ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    }

    void PointShadowMap::EndAllFaces(ID3D11DeviceContext* ctx,
        ID3D11RenderTargetView* mainRTV,
        ID3D11DepthStencilView* mainDSV,
        int vpWidth, int vpHeight)
    {
        if (!ctx) return;

        ctx->OMSetRenderTargets(1, &mainRTV, mainDSV);

        D3D11_VIEWPORT vp{ 0.0f, 0.0f, static_cast<float>(vpWidth), static_cast<float>(vpHeight), 0.0f, 1.0f };
        ctx->RSSetViewports(1, &vp);
    }

    void PointShadowMap::BindForSampling(ID3D11DeviceContext* ctx, int srvSlot, int samplerSlot)
    {
        if (!ctx) return;

        ctx->PSSetShaderResources(srvSlot, 1, m_srv.GetAddressOf());
        ctx->PSSetSamplers(samplerSlot, 1, m_sampler.GetAddressOf());
    }

    XMMATRIX PointShadowMap::GetFaceMatrix(int l, int f) const
    {
        if (l < 0 || l >= m_maxLights || f < 0 || f >= 6)
            return XMMatrixIdentity();
        return m_faceMatrices[l * 6 + f];
    }

    XMFLOAT3 PointShadowMap::GetLightPosition(int l) const
    {
        if (l < 0 || l >= m_maxLights) return {};
        return m_lights[l].position;
    }

    float PointShadowMap::GetLightRadius(int l) const
    {
        if (l < 0 || l >= m_maxLights) return 0.0f;
        return m_lights[l].radius;
    }

    void PointShadowMap::Shutdown()
    {
        m_colorTex.Reset();
        m_srv.Reset();
        m_sampler.Reset();
        m_rtvs.clear();
        m_depthTex.Reset();
        m_depthDSV.Reset();
        m_depthStencilState.Reset();
        m_rasterizerState.Reset();

        LOG_INFO("PointShadowMap shut down.");
    }
}
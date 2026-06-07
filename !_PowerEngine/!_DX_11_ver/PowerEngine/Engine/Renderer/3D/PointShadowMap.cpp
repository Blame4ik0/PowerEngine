#include "PointShadowMap.h"
#include "Core/Logger.h"

using namespace DirectX;

namespace Engine
{
    static const XMFLOAT3 s_faceTargets[6] =
    {
        {  1,  0,  0 }, // +X
        { -1,  0,  0 }, // -X
        {  0,  1,  0 }, // +Y
        {  0, -1,  0 }, // -Y
        {  0,  0,  1 }, // +Z
        {  0,  0, -1 }, // -Z
    };
    static const XMFLOAT3 s_faceUps[6] =
    {
        {  0,  1,  0 },
        {  0,  1,  0 },
        {  0,  0, -1 },
        {  0,  0,  1 },
        {  0,  1,  0 },
        {  0,  1,  0 },
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
        }
    }

    bool PointShadowMap::Init(ID3D11Device* device,
        const std::wstring& shaderPath,
        int maxLights,
        int faceResolution)
    {
        m_maxLights = maxLights;
        m_resolution = faceResolution;

        if (!m_shader.Load(device, shaderPath, "VS_Main", "PS_Main"))
            return false;

        m_lights.resize(maxLights);
        m_faceMatrices.resize(maxLights * 6, XMMatrixIdentity());

        if (!ReinitTextures(device))
            return false;

        // Linear sampler — no comparison needed, just fetch stored depth
        D3D11_SAMPLER_DESC sampDesc{};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

        if (FAILED(device->CreateSamplerState(&sampDesc,
            m_sampler.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateSamplerState failed.");
            return false;
        }

        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
        dsDesc.StencilEnable = FALSE;

        if (FAILED(device->CreateDepthStencilState(&dsDesc,
            m_depthStencilState.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateDepthStencilState failed.");
            return false;
        }

        D3D11_RASTERIZER_DESC rDesc{};
        rDesc.FillMode = D3D11_FILL_SOLID;
        rDesc.CullMode = D3D11_CULL_BACK;
        rDesc.FrontCounterClockwise = FALSE;
        rDesc.DepthClipEnable = TRUE;
        rDesc.DepthBias = 0;
        rDesc.SlopeScaledDepthBias = 0.0f;

        if (FAILED(device->CreateRasterizerState(&rDesc,
            m_rasterizerState.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateRasterizerState failed.");
            return false;
        }

        m_viewport.TopLeftX = 0.0f;
        m_viewport.TopLeftY = 0.0f;
        m_viewport.Width = static_cast<float>(m_resolution);
        m_viewport.Height = static_cast<float>(m_resolution);
        m_viewport.MinDepth = 0.0f;
        m_viewport.MaxDepth = 1.0f;

        LOG_INFO("PointShadowMap initialized ({} lights, {}x{} per face).",
            maxLights, m_resolution, m_resolution);
        return true;
    }

    bool PointShadowMap::ReinitTextures(ID3D11Device* device)
    {
        m_colorCubeArray.Reset();
        m_srv.Reset();
        m_depthBuffer.Reset();
        m_depthDSV.Reset();
        m_rtvs.clear();

        // Color R32F cube array — stores linear depth
        D3D11_TEXTURE2D_DESC colDesc{};
        colDesc.Width = static_cast<UINT>(m_resolution);
        colDesc.Height = static_cast<UINT>(m_resolution);
        colDesc.MipLevels = 1;
        colDesc.ArraySize = static_cast<UINT>(m_maxLights * 6);
        colDesc.Format = DXGI_FORMAT_R32_FLOAT;
        colDesc.SampleDesc = { 1, 0 };
        colDesc.Usage = D3D11_USAGE_DEFAULT;
        colDesc.BindFlags = D3D11_BIND_RENDER_TARGET |
            D3D11_BIND_SHADER_RESOURCE;
        colDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        if (FAILED(device->CreateTexture2D(&colDesc, nullptr,
            m_colorCubeArray.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateTexture2D (color) failed.");
            return false;
        }

        // One RTV per face per light
        m_rtvs.resize(m_maxLights * 6);
        for (int light = 0; light < m_maxLights; light++)
        {
            for (int face = 0; face < 6; face++)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
                rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.MipSlice = 0;
                rtvDesc.Texture2DArray.FirstArraySlice = light * 6 + face;
                rtvDesc.Texture2DArray.ArraySize = 1;

                if (FAILED(device->CreateRenderTargetView(
                    m_colorCubeArray.Get(), &rtvDesc,
                    m_rtvs[light * 6 + face].GetAddressOf())))
                {
                    LOG_ERROR("PointShadowMap: CreateRTV failed l{} f{}.",
                        light, face);
                    return false;
                }
            }
        }

        // SRV — cube map array
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
        srvDesc.TextureCubeArray.MostDetailedMip = 0;
        srvDesc.TextureCubeArray.MipLevels = 1;
        srvDesc.TextureCubeArray.First2DArrayFace = 0;
        srvDesc.TextureCubeArray.NumCubes = m_maxLights;

        if (FAILED(device->CreateShaderResourceView(
            m_colorCubeArray.Get(), &srvDesc,
            m_srv.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateSRV failed.");
            return false;
        }

        // Shared depth buffer — reused for all faces
        D3D11_TEXTURE2D_DESC depthDesc{};
        depthDesc.Width = static_cast<UINT>(m_resolution);
        depthDesc.Height = static_cast<UINT>(m_resolution);
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthDesc.SampleDesc = { 1, 0 };
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        if (FAILED(device->CreateTexture2D(&depthDesc, nullptr,
            m_depthBuffer.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateTexture2D (depth) failed.");
            return false;
        }

        if (FAILED(device->CreateDepthStencilView(m_depthBuffer.Get(),
            nullptr, m_depthDSV.GetAddressOf())))
        {
            LOG_ERROR("PointShadowMap: CreateDSV failed.");
            return false;
        }

        return true;
    }

    void PointShadowMap::UpdateLight(int lightIndex,
        const XMFLOAT3& position,
        float radius)
    {
        if (lightIndex < 0 || lightIndex >= m_maxLights) return;

        m_lights[lightIndex].position = position;
        m_lights[lightIndex].radius = radius;

        XMVECTOR pos = XMLoadFloat3(&position);
        XMMATRIX proj = XMMatrixPerspectiveFovLH(
            XM_PIDIV2, 1.0f, 0.01f, radius);

        for (int face = 0; face < 6; face++)
        {
            XMVECTOR target = pos + XMLoadFloat3(&s_faceTargets[face]);
            XMVECTOR up = XMLoadFloat3(&s_faceUps[face]);
            XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
            m_faceMatrices[lightIndex * 6 + face] = view * proj;
        }
    }

    void PointShadowMap::BeginFace(ID3D11DeviceContext* ctx,
        int lightIndex, int face)
    {
        int idx = lightIndex * 6 + face;
        if (idx < 0 || idx >= (int)m_rtvs.size()) return;

        // Unbind SRV
        ID3D11ShaderResourceView* nullSRV = nullptr;
        ctx->PSSetShaderResources(6, 1, &nullSRV);

        // Clear color to max depth (1.0) and depth buffer
        float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        ctx->ClearRenderTargetView(m_rtvs[idx].Get(), clearColor);
        ctx->ClearDepthStencilView(m_depthDSV.Get(),
            D3D11_CLEAR_DEPTH, 1.0f, 0);

        // Bind color RTV + shared depth buffer
        ctx->OMSetRenderTargets(1, m_rtvs[idx].GetAddressOf(),
            m_depthDSV.Get());
        ctx->RSSetViewports(1, &m_viewport);
        ctx->RSSetState(m_rasterizerState.Get());
        ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    }

    void PointShadowMap::EndAllFaces(ID3D11DeviceContext* ctx,
        ID3D11RenderTargetView* mainRTV,
        ID3D11DepthStencilView* mainDSV,
        int vpWidth, int vpHeight)
    {
        ctx->OMSetRenderTargets(1, &mainRTV, mainDSV);

        D3D11_VIEWPORT vp{};
        vp.Width = static_cast<float>(vpWidth);
        vp.Height = static_cast<float>(vpHeight);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vp);
    }

    void PointShadowMap::BindForSampling(ID3D11DeviceContext* ctx,
        int srvSlot, int samplerSlot)
    {
        ctx->PSSetShaderResources(srvSlot, 1, m_srv.GetAddressOf());
        ctx->PSSetSamplers(samplerSlot, 1, m_sampler.GetAddressOf());
    }

    XMMATRIX PointShadowMap::GetFaceMatrix(int light, int face) const
    {
        return m_faceMatrices[light * 6 + face];
    }

    float PointShadowMap::GetLightRadius(int light) const
    {
        return m_lights[light].radius;
    }

    XMFLOAT3 PointShadowMap::GetLightPosition(int light) const
    {
        return m_lights[light].position;
    }

    void PointShadowMap::Shutdown()
    {
        m_colorCubeArray.Reset();
        m_srv.Reset();
        m_sampler.Reset();
        m_depthBuffer.Reset();
        m_depthDSV.Reset();
        m_depthStencilState.Reset();
        m_rasterizerState.Reset();
        m_rtvs.clear();
        LOG_INFO("PointShadowMap shut down.");
    }
}
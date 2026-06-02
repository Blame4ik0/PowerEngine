#include "ShadowMap.h"
#include "Core/Logger.h"
#include <cmath>

using namespace DirectX;

namespace Engine
{
    bool ShadowMap::Init(ID3D11Device* device,
        const std::wstring& shaderPath,
        int resolution)
    {
        m_resolution = resolution;

        if (!m_shader.Load(device, shaderPath, "VS_Main", "PS_Main"))
            return false;

        // R32_TYPELESS so we can use it as both D32_FLOAT (DSV)
        // and R32_FLOAT (SRV) — same underlying texture, two views
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = static_cast<UINT>(resolution);
        texDesc.Height = static_cast<UINT>(resolution);
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL |
            D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = device->CreateTexture2D(&texDesc, nullptr,
            m_depthTexture.GetAddressOf());
        if (FAILED(hr))
        {
            LOG_ERROR("ShadowMap: CreateTexture2D failed. HRESULT: {:#x}",
                (unsigned)hr);
            return false;
        }

        // DSV — write depth during shadow pass
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;

        hr = device->CreateDepthStencilView(m_depthTexture.Get(), &dsvDesc,
            m_dsv.GetAddressOf());
        if (FAILED(hr))
        {
            LOG_ERROR("ShadowMap: CreateDSV failed. HRESULT: {:#x}", (unsigned)hr);
            return false;
        }

        // SRV — read depth during main pass
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        hr = device->CreateShaderResourceView(m_depthTexture.Get(), &srvDesc,
            m_srv.GetAddressOf());
        if (FAILED(hr))
        {
            LOG_ERROR("ShadowMap: CreateSRV failed. HRESULT: {:#x}", (unsigned)hr);
            return false;
        }

        // Comparison sampler for PCF hardware filtering
        D3D11_SAMPLER_DESC sampDesc{};
        sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        sampDesc.BorderColor[0] = 1.0f; // areas outside shadow map = fully lit
        sampDesc.BorderColor[1] = 1.0f;
        sampDesc.BorderColor[2] = 1.0f;
        sampDesc.BorderColor[3] = 1.0f;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

        hr = device->CreateSamplerState(&sampDesc, m_sampler.GetAddressOf());
        if (FAILED(hr))
        {
            LOG_ERROR("ShadowMap: CreateSamplerState failed. HRESULT: {:#x}",
                (unsigned)hr);
            return false;
        }

        // Rasterizer with depth bias — prevents shadow acne
        // DepthBias offsets depth values written to the shadow map
        // so the surface doesn't shadow itself
        D3D11_RASTERIZER_DESC rDesc{};
        rDesc.FillMode = D3D11_FILL_SOLID;
        rDesc.CullMode = D3D11_CULL_BACK;
        rDesc.FrontCounterClockwise = FALSE;
        rDesc.DepthClipEnable = TRUE;
        rDesc.DepthBias = 1000;
        rDesc.DepthBiasClamp = 0.0f;
        rDesc.SlopeScaledDepthBias = 1.0f;

        hr = device->CreateRasterizerState(&rDesc, m_rasterizerState.GetAddressOf());
        if (FAILED(hr))
        {
            LOG_ERROR("ShadowMap: CreateRasterizerState failed. HRESULT: {:#x}",
                (unsigned)hr);
            return false;
        }

        m_viewport.TopLeftX = 0.0f;
        m_viewport.TopLeftY = 0.0f;
        m_viewport.Width = static_cast<float>(resolution);
        m_viewport.Height = static_cast<float>(resolution);
        m_viewport.MinDepth = 0.0f;
        m_viewport.MaxDepth = 1.0f;

        m_lightSpaceMatrix = XMMatrixIdentity();

        LOG_INFO("ShadowMap initialized ({}x{}).", resolution, resolution);
        return true;
    }

    void ShadowMap::UpdateLightSpace(const XMFLOAT3& lightDir, float sceneRadius)
    {
        XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&lightDir));

        // Place light camera behind the scene in the light direction
        XMVECTOR pos = XMVectorScale(XMVectorNegate(dir), sceneRadius * 2.0f);
        XMVECTOR target = XMVectorZero();
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        // If light points straight down/up use a different up vector
        XMFLOAT3 dirF;
        XMStoreFloat3(&dirF, dir);
        if (fabsf(dirF.y) > 0.99f)
            up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

        XMMATRIX view = XMMatrixLookAtLH(pos, target, up);

        // Orthographic projection sized to cover the scene
        XMMATRIX proj = XMMatrixOrthographicLH(
            sceneRadius * 2.0f,
            sceneRadius * 2.0f,
            0.1f,
            sceneRadius * 4.0f);

        m_lightSpaceMatrix = view * proj;
    }

    void ShadowMap::BeginShadowPass(ID3D11DeviceContext* ctx)
    {
        // Unbind shadow SRV from pixel shader to avoid
        // resource-still-bound warnings from DX11 debug layer
        ID3D11ShaderResourceView* nullSRV = nullptr;
        ctx->PSSetShaderResources(5, 1, &nullSRV);

        // No color target — depth only
        ID3D11RenderTargetView* nullRTV = nullptr;
        ctx->OMSetRenderTargets(0, &nullRTV, m_dsv.Get());
        ctx->ClearDepthStencilView(m_dsv.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        ctx->RSSetViewports(1, &m_viewport);
        ctx->RSSetState(m_rasterizerState.Get());
    }

    void ShadowMap::EndShadowPass(ID3D11DeviceContext* ctx,
        ID3D11RenderTargetView* mainRTV,
        ID3D11DepthStencilView* mainDSV,
        int viewportWidth, int viewportHeight)
    {
        // Restore main render target
        ctx->OMSetRenderTargets(1, &mainRTV, mainDSV);

        D3D11_VIEWPORT vp{};
        vp.TopLeftX = 0.0f;
        vp.TopLeftY = 0.0f;
        vp.Width = static_cast<float>(viewportWidth);
        vp.Height = static_cast<float>(viewportHeight);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vp);
    }

    void ShadowMap::BindForSampling(ID3D11DeviceContext* ctx,
        int srvSlot, int samplerSlot)
    {
        ctx->PSSetShaderResources(srvSlot, 1, m_srv.GetAddressOf());
        ctx->PSSetSamplers(samplerSlot, 1, m_sampler.GetAddressOf());
    }

    void ShadowMap::Shutdown()
    {
        m_depthTexture.Reset();
        m_dsv.Reset();
        m_srv.Reset();
        m_sampler.Reset();
        m_rasterizerState.Reset();
        LOG_INFO("ShadowMap shut down.");
    }
}

#include "ShadowMap.h"
#include "Core/Logger.h"

using namespace DirectX;

namespace Engine
{
    bool ShadowMap::Init(ID3D11Device* device, const std::wstring& shaderPath, int resolution)
    {
        if (resolution > 0)
            m_resolution = resolution;

        if (!m_shader.Load(device, shaderPath, "VS_Main", "PS_Main"))
            return false;

        // Texture
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = resolution;
        texDesc.Height = resolution;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

        device->CreateTexture2D(&texDesc, nullptr, m_depthTexture.GetAddressOf());

        // DSV
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(m_depthTexture.Get(), &dsvDesc, m_dsv.GetAddressOf());

        // SRV
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_depthTexture.Get(), &srvDesc, m_srv.GetAddressOf());

        // Comparison Sampler
        D3D11_SAMPLER_DESC sampDesc{};
        sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        sampDesc.BorderColor[0] = 1.0f;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        device->CreateSamplerState(&sampDesc, m_sampler.GetAddressOf());

        // Rasterizer with bias
        D3D11_RASTERIZER_DESC rDesc{};
        rDesc.FillMode = D3D11_FILL_SOLID;
        rDesc.CullMode = D3D11_CULL_BACK;
        rDesc.DepthBias = 100;
        rDesc.SlopeScaledDepthBias = 0.5f;
        rDesc.DepthClipEnable = TRUE;
        device->CreateRasterizerState(&rDesc, m_rasterizerState.GetAddressOf());

        // Depth stencil state
        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
        dsDesc.StencilEnable = FALSE;
        device->CreateDepthStencilState(&dsDesc, m_depthStencilState.GetAddressOf());

        m_viewport.Width = (float)resolution;
        m_viewport.Height = (float)resolution;
        m_viewport.MinDepth = 0.0f;
        m_viewport.MaxDepth = 1.0f;

        LOG_INFO("ShadowMap initialized ({}x{})", resolution, resolution);
        return true;
    }

    void ShadowMap::UpdateLightSpace(const XMFLOAT3& lightDir, float sceneRadius)
    {
        XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&lightDir));
        XMVECTOR center = XMVectorSet(0.0f, 2.0f, 0.0f, 0.0f);

        XMMATRIX view = XMMatrixLookAtLH(center - dir * sceneRadius * 1.8f, center, XMVectorSet(0, 1, 0, 0));
        XMMATRIX proj = XMMatrixOrthographicLH(sceneRadius * 2.3f, sceneRadius * 2.3f, 0.5f, sceneRadius * 5.0f);

        m_lightSpaceMatrix = view * proj;
    }

    void ShadowMap::BeginShadowPass(ID3D11DeviceContext* ctx)
    {
        ID3D11ShaderResourceView* nullSRV = nullptr;
        ctx->PSSetShaderResources(5, 1, &nullSRV);

        ctx->OMSetRenderTargets(0, nullptr, m_dsv.Get());
        ctx->ClearDepthStencilView(m_dsv.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        ctx->RSSetViewports(1, &m_viewport);
        ctx->RSSetState(m_rasterizerState.Get());
        ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    }

    void ShadowMap::EndShadowPass(ID3D11DeviceContext* ctx,
        ID3D11RenderTargetView* mainRTV,
        ID3D11DepthStencilView* mainDSV,
        int vpW, int vpH)
    {
        ctx->OMSetRenderTargets(1, &mainRTV, mainDSV);

        D3D11_VIEWPORT vp{ 0,0,(float)vpW,(float)vpH,0.0f,1.0f };
        ctx->RSSetViewports(1, &vp);
    }

    void ShadowMap::BindForSampling(ID3D11DeviceContext* ctx, int srvSlot, int samplerSlot)
    {
        ctx->PSSetShaderResources(srvSlot, 1, m_srv.GetAddressOf());
        ctx->PSSetSamplers(samplerSlot, 1, m_sampler.GetAddressOf());
    }

    void ShadowMap::SetQuality(ShadowQuality quality)
    {
        m_quality = quality;
        switch (quality)
        {
        case ShadowQuality::Low:
            m_resolution = 512;
            m_pcfRadius = 0;
            break;
        case ShadowQuality::Medium:
            m_resolution = 1024;
            m_pcfRadius = 1;
            break;
        case ShadowQuality::High:
            m_resolution = 2048;
            m_pcfRadius = 1;
            break;
        case ShadowQuality::Ultra:
            m_resolution = 4096;
            m_pcfRadius = 2;
            break;
        case ShadowQuality::Cinematic:
            m_resolution = 8192;
            m_pcfRadius = 3;
            break;
        }
    }

    void ShadowMap::Shutdown()
    {
        m_depthTexture.Reset();
        m_dsv.Reset();
        m_srv.Reset();
        m_sampler.Reset();
        m_rasterizerState.Reset();
        m_depthStencilState.Reset();
    }
}
#include "RenderContext.h"
#include "Core/Logger.h"
#include <stdexcept>
#include <string>

#define DX_CHECK(hr, msg) \
    if (FAILED(hr)) throw std::runtime_error(std::string(msg) + \
        " HRESULT: " + std::to_string(hr))

namespace Engine
{
    RenderContext::RenderContext(void* hwnd, int width, int height, bool vsync, int refreshRate)
        : m_vsync(vsync), m_width(width), m_height(height), m_refreshRate(refreshRate)
    {
        DXGI_SWAP_CHAIN_DESC scd{};
        scd.BufferCount = 2;
        scd.BufferDesc.Width = width;
        scd.BufferDesc.Height = height;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferDesc.RefreshRate.Numerator = static_cast<UINT>(refreshRate);
        scd.BufferDesc.RefreshRate.Denominator = 1;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = static_cast<HWND>(hwnd);
        scd.SampleDesc.Count = 1;
        scd.Windowed = TRUE;
        scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1,
                                             D3D_FEATURE_LEVEL_11_0 };

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            flags, levels, 2, D3D11_SDK_VERSION,
            &scd, &m_swapChain, &m_device, &featureLevel, &m_deviceContext
        );
        DX_CHECK(hr, "D3D11CreateDeviceAndSwapChain failed.");

        CreateRenderTargetAndDepth();
        LOG_INFO("RenderContext initialized ({}x{}, vsync={}, {}hz)",
            width, height, vsync, refreshRate);
    }

    void RenderContext::CreateRenderTargetAndDepth()
    {
        ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
            reinterpret_cast<void**>(backBuffer.GetAddressOf()));
        DX_CHECK(hr, "GetBuffer failed.");

        hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr,
            m_renderTargetView.GetAddressOf());
        DX_CHECK(hr, "CreateRenderTargetView failed.");

        D3D11_TEXTURE2D_DESC dsDesc{};
        dsDesc.Width = m_width;
        dsDesc.Height = m_height;
        dsDesc.MipLevels = 1;
        dsDesc.ArraySize = 1;
        dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsDesc.SampleDesc.Count = 1;
        dsDesc.Usage = D3D11_USAGE_DEFAULT;
        dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        hr = m_device->CreateTexture2D(&dsDesc, nullptr,
            m_depthStencilBuffer.GetAddressOf());
        DX_CHECK(hr, "CreateTexture2D (depth) failed.");

        hr = m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr,
            m_depthStencilView.GetAddressOf());
        DX_CHECK(hr, "CreateDepthStencilView failed.");

        m_deviceContext->OMSetRenderTargets(1,
            m_renderTargetView.GetAddressOf(),
            m_depthStencilView.Get());

        D3D11_VIEWPORT vp{};
        vp.Width = static_cast<float>(m_width);
        vp.Height = static_cast<float>(m_height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        m_deviceContext->RSSetViewports(1, &vp);
    }

    void RenderContext::ReleaseRenderTargetAndDepth()
    {
        m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        m_renderTargetView.Reset();
        m_depthStencilView.Reset();
        m_depthStencilBuffer.Reset();
    }

    void RenderContext::BeginFrame(float r, float g, float b, float a)
    {
        float color[4] = { r, g, b, a };
        m_deviceContext->ClearRenderTargetView(m_renderTargetView.Get(), color);
        m_deviceContext->ClearDepthStencilView(m_depthStencilView.Get(),
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }

    void RenderContext::EndFrame()
    {
        m_swapChain->Present(m_vsync ? 1 : 0, 0);
    }

    void RenderContext::Resize(int width, int height)
    {
        if (width == m_width && height == m_height) return;
        m_width = width;
        m_height = height;

        ReleaseRenderTargetAndDepth();
        HRESULT hr = m_swapChain->ResizeBuffers(0, width, height,
            DXGI_FORMAT_UNKNOWN, 0);
        DX_CHECK(hr, "ResizeBuffers failed.");
        CreateRenderTargetAndDepth();
        LOG_INFO("RenderContext resized: {}x{}", width, height);
    }
}
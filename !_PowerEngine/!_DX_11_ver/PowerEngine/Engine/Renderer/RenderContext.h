#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace Engine
{
    using Microsoft::WRL::ComPtr;

    class RenderContext
    {
    public:
        RenderContext(void* hwnd, int width, int height, bool vsync, int refreshRate = 60);
        ~RenderContext() = default;

        RenderContext(const RenderContext&) = delete;
        RenderContext& operator=(const RenderContext&) = delete;

        void BeginFrame(float r, float g, float b, float a = 1.0f);
        void EndFrame();
        void Resize(int width, int height);

        ID3D11Device* GetDevice()       const { return m_device.Get(); }
        ID3D11DeviceContext* GetDeviceContext() const { return m_deviceContext.Get(); }
        IDXGISwapChain* GetSwapChain()    const { return m_swapChain.Get(); }

    private:
        void CreateRenderTargetAndDepth();
        void ReleaseRenderTargetAndDepth();

        ComPtr<ID3D11Device>            m_device;
        ComPtr<ID3D11DeviceContext>     m_deviceContext;
        ComPtr<IDXGISwapChain>          m_swapChain;
        ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
        ComPtr<ID3D11DepthStencilView>  m_depthStencilView;
        ComPtr<ID3D11Texture2D>         m_depthStencilBuffer;

        bool m_vsync = true;
        int  m_width = 0;
        int  m_height = 0;
        int  m_refreshRate = 60;
    };
}
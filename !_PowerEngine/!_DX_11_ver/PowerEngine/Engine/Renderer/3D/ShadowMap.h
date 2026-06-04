#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include "Renderer/Shader.h"

namespace Engine
{
    using Microsoft::WRL::ComPtr;

    enum class ShadowQuality
    {
        Low,      // 512x512,  no PCF
        Medium,   // 1024x1024, PCF 2x2
        High,     // 2048x2048, PCF 3x3
        Ultra,    // 4096x4096, PCF 5x5
        Cinematic // 8192x8192, PCF 7x7
    };

    class ShadowMap
    {
    public:
        ShadowMap() = default;
        ~ShadowMap() = default;

        bool Init(ID3D11Device* device, const std::wstring& shaderPath, int resolution = 2048);
        void Shutdown();

        void BeginShadowPass(ID3D11DeviceContext* ctx);
        void EndShadowPass(ID3D11DeviceContext* ctx,
            ID3D11RenderTargetView* mainRTV,
            ID3D11DepthStencilView* mainDSV,
            int viewportWidth, int viewportHeight);

        void UpdateLightSpace(const DirectX::XMFLOAT3& lightDir, float sceneRadius = 60.0f);

        void BindForSampling(ID3D11DeviceContext* ctx, int srvSlot = 5, int samplerSlot = 1);
        Shader& GetShader() { return m_shader; }

        DirectX::XMMATRIX GetLightSpaceMatrix() const { return m_lightSpaceMatrix; }

        void SetQuality(ShadowQuality quality);
        ShadowQuality GetQuality() const { return m_quality; }
        int GetPCFRadius() const { return m_pcfRadius; }
        int GetResolution() const { return m_resolution; }

    private:
        Shader                           m_shader;
        ComPtr<ID3D11Texture2D>          m_depthTexture;
        ComPtr<ID3D11DepthStencilView>   m_dsv;
        ComPtr<ID3D11ShaderResourceView> m_srv;
        ComPtr<ID3D11SamplerState>       m_sampler;
        ComPtr<ID3D11RasterizerState>    m_rasterizerState;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;

        DirectX::XMMATRIX                m_lightSpaceMatrix;
        D3D11_VIEWPORT                   m_viewport{};
        int                              m_resolution = 2048;

        ShadowQuality m_quality = ShadowQuality::Ultra;
        int           m_pcfRadius = 1;
    };
}
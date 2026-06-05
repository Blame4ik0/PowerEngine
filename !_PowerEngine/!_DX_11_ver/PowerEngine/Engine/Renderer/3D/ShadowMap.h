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
        Low,       // 512,  PCF 0 (no filter)
        Medium,    // 1024, PCF 1x1
        High,      // 2048, PCF 3x3
        Ultra,     // 4096, PCF 5x5
        Cinematic  // 8192, PCF 7x7
    };

    class ShadowMap
    {
    public:
        ShadowMap() = default;
        ~ShadowMap() = default;

        ShadowMap(const ShadowMap&) = delete;
        ShadowMap& operator=(const ShadowMap&) = delete;

        bool Init(ID3D11Device* device,
            const std::wstring& shaderPath,
            int resolution = 2048);
        void Shutdown();

        void BeginShadowPass(ID3D11DeviceContext* ctx);
        void EndShadowPass(ID3D11DeviceContext* ctx,
            ID3D11RenderTargetView* mainRTV,
            ID3D11DepthStencilView* mainDSV,
            int viewportWidth, int viewportHeight);

        void UpdateLightSpace(const DirectX::XMFLOAT3& lightDir,
            float sceneRadius = 20.0f);

        void BindForSampling(ID3D11DeviceContext* ctx,
            int srvSlot = 5, int samplerSlot = 1);

        // Quality — call before Init or triggers reinit via Renderer3D
        void SetQuality(ShadowQuality quality);
        ShadowQuality GetQuality()    const { return m_quality; }
        int           GetResolution() const { return m_resolution; }
        int           GetPCFRadius()  const { return m_pcfRadius; }

        Shader& GetShader() { return m_shader; }
        DirectX::XMMATRIX         GetLightSpaceMatrix()  const { return m_lightSpaceMatrix; }
        ID3D11ShaderResourceView* GetSRV()               const { return m_srv.Get(); }

    private:
        Shader                           m_shader;
        ComPtr<ID3D11Texture2D>          m_depthTexture;
        ComPtr<ID3D11DepthStencilView>   m_dsv;
        ComPtr<ID3D11ShaderResourceView> m_srv;
        ComPtr<ID3D11SamplerState>       m_sampler;
        ComPtr<ID3D11RasterizerState>    m_rasterizerState;
        ComPtr<ID3D11DepthStencilState>  m_depthStencilState;

        DirectX::XMMATRIX m_lightSpaceMatrix;
        D3D11_VIEWPORT    m_viewport{};

        ShadowQuality m_quality = ShadowQuality::High;
        int           m_resolution = 2048;
        int           m_pcfRadius = 1;
    };
}
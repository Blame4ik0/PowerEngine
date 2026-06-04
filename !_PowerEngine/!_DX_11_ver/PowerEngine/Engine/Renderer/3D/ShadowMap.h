#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include "Renderer/Shader.h"

namespace Engine
{
    using Microsoft::WRL::ComPtr;

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
    };
}
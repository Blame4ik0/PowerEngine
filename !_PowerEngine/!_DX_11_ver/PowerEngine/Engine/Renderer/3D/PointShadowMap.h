#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include "Renderer/Shader.h"
#include <vector>

namespace Engine
{
    using Microsoft::WRL::ComPtr;

    enum class PointShadowQuality
    {
        Low,    // 256x256
        Medium, // 512x512
        High,   // 1024x1024
        Ultra   // 2048x2048
    };

    class PointShadowMap
    {
    public:
        PointShadowMap() = default;
        ~PointShadowMap() = default;

        PointShadowMap(const PointShadowMap&) = delete;
        PointShadowMap& operator=(const PointShadowMap&) = delete;

        bool Init(ID3D11Device* device,
            const std::wstring& shaderPath,
            int maxLights = 4,
            int faceResolution = 512);
        void Shutdown();

        void BeginFace(ID3D11DeviceContext* ctx,
            int lightIndex, int face);

        void EndAllFaces(ID3D11DeviceContext* ctx,
            ID3D11RenderTargetView* mainRTV,
            ID3D11DepthStencilView* mainDSV,
            int vpWidth, int vpHeight);

        void UpdateLight(int lightIndex,
            const DirectX::XMFLOAT3& position,
            float radius);

        void BindForSampling(ID3D11DeviceContext* ctx,
            int srvSlot = 6,
            int samplerSlot = 2);

        void SetQuality(PointShadowQuality quality);
        PointShadowQuality GetQuality()    const { return m_quality; }
        int                GetResolution() const { return m_resolution; }
        int                GetMaxLights()  const { return m_maxLights; }

        Shader& GetShader() { return m_shader; }
        DirectX::XMMATRIX GetFaceMatrix(int light, int face)   const;
        float             GetLightRadius(int light)            const;
        DirectX::XMFLOAT3 GetLightPosition(int light)         const;

    private:
        bool ReinitTextures(ID3D11Device* device);

        Shader m_shader;

        // Color cube array — stores linear depth as R32F
        ComPtr<ID3D11Texture2D>          m_colorCubeArray;
        ComPtr<ID3D11ShaderResourceView> m_srv;
        ComPtr<ID3D11SamplerState>       m_sampler;

        // One RTV per face per light
        std::vector<ComPtr<ID3D11RenderTargetView>> m_rtvs;

        // Shared depth buffer for depth testing during shadow pass
        ComPtr<ID3D11Texture2D>         m_depthBuffer;
        ComPtr<ID3D11DepthStencilView>  m_depthDSV;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;
        ComPtr<ID3D11RasterizerState>   m_rasterizerState;

        D3D11_VIEWPORT     m_viewport{};
        PointShadowQuality m_quality = PointShadowQuality::Medium;
        int                m_resolution = 512;
        int                m_maxLights = 4;

        struct LightData
        {
            DirectX::XMFLOAT3 position = {};
            float             radius = 10.0f;
        };
        std::vector<LightData>          m_lights;
        std::vector<DirectX::XMMATRIX>  m_faceMatrices;
    };
}
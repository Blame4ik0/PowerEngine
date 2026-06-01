#pragma once
#include "Renderer/RenderContext.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture2D.h"
#include "Mesh.h"
#include "Camera3D.h"
#include "Light.h"
#include "ShadowMap.h"
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Engine
{
    enum class RenderPass { Shadow, Main };

    class Renderer3D
    {
    public:
        static constexpr int MaxPointLights = 4;

        Renderer3D() = default;
        ~Renderer3D() = default;

        Renderer3D(const Renderer3D&) = delete;
        Renderer3D& operator=(const Renderer3D&) = delete;

        bool Init(RenderContext* context, const std::wstring& shaderPath);
        void Shutdown();

        void BeginScene(const Camera3D& camera);

        void SetDirectionalLight(const DirectionalLight& light);
        void AddPointLight(const PointLight& light);
        void ClearPointLights();

        void DrawMesh(const Mesh& mesh,
            const DirectX::XMMATRIX& worldMatrix,
            const Material& material = Material{});

        void DrawMesh(const Mesh& mesh,
            float x, float y, float z,
            float rotX = 0, float rotY = 0, float rotZ = 0,
            float scaleX = 1, float scaleY = 1, float scaleZ = 1,
            const Material& material = Material{});

        void BeginShadowPass();
        void EndShadowPass();

        void EnableShadows(bool enabled) { m_shadowsEnabled = enabled; }
        ShadowMap& GetShadowMap() { return m_shadowMap; }

        void OnResize(float aspectRatio);

    private:
        void UpdateLightBuffer();
        Texture2D* GetOrLoadTexture(const std::string& path);

        RenderContext* m_context = nullptr;
        Shader                          m_shader;

        ComPtr<ID3D11Buffer>            m_cbPerObject;
        ComPtr<ID3D11Buffer>            m_cbPerFrame;
        ComPtr<ID3D11Buffer>            m_cbLight;
        ComPtr<ID3D11Buffer>            m_cbMaterial;
        ComPtr<ID3D11Buffer>            m_cbShadow;
        ComPtr<ID3D11Buffer>            m_cbShadowPass;
        ComPtr<ID3D11InputLayout>       m_inputLayout;
        ComPtr<ID3D11RasterizerState>   m_rasterizerState;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;
        ComPtr<ID3D11SamplerState>      m_sampler;

        DirectX::XMMATRIX               m_view;
        DirectX::XMMATRIX               m_projection;
        DirectX::XMFLOAT3               m_cameraPosition;

        DirectionalLight                m_dirLight;
        std::vector<PointLight>         m_pointLights;

        ShadowMap                       m_shadowMap;
        RenderPass                      m_currentPass = RenderPass::Main;
        bool                            m_shadowsEnabled = true;

        std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_textureCache;
        std::shared_ptr<Texture2D>      m_whiteTexture;
    };
}
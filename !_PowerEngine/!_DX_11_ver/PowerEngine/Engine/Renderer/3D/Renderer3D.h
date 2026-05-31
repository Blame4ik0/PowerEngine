#pragma once
#include "Renderer/RenderContext.h"
#include "Renderer/Shader.h"
#include "Mesh.h"
#include "Camera3D.h"
#include "Light.h"
#include <DirectXMath.h>
#include <vector>

namespace Engine
{
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

        // Lighting — call before DrawMesh
        void SetDirectionalLight(const DirectionalLight& light);
        void AddPointLight(const PointLight& light);
        void ClearPointLights();

        // Draw with material
        void DrawMesh(const Mesh& mesh,
            const DirectX::XMMATRIX& worldMatrix,
            const Material& material = Material{});

        void DrawMesh(const Mesh& mesh,
            float x, float y, float z,
            float rotX = 0, float rotY = 0, float rotZ = 0,
            float scaleX = 1, float scaleY = 1, float scaleZ = 1,
            const Material& material = Material{});

        void OnResize(float aspectRatio);

    private:
        void UpdateLightBuffer();

        RenderContext* m_context = nullptr;
        Shader                          m_shader;

        ComPtr<ID3D11Buffer>            m_cbPerObject;
        ComPtr<ID3D11Buffer>            m_cbPerFrame;
        ComPtr<ID3D11Buffer>            m_cbLight;
        ComPtr<ID3D11Buffer>            m_cbMaterial;
        ComPtr<ID3D11InputLayout>       m_inputLayout;
        ComPtr<ID3D11RasterizerState>   m_rasterizerState;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;

        DirectX::XMMATRIX               m_view;
        DirectX::XMMATRIX               m_projection;
        DirectX::XMFLOAT3               m_cameraPosition;

        DirectionalLight                m_dirLight;
        std::vector<PointLight>         m_pointLights;
    };
}
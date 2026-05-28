#pragma once
#include "Renderer/RenderContext.h"
#include "Renderer/Shader.h"
#include "Mesh.h"
#include "Camera3D.h"
#include <DirectXMath.h>

namespace Engine
{
    class Renderer3D
    {
    public:
        Renderer3D() = default;
        ~Renderer3D() = default;

        Renderer3D(const Renderer3D&) = delete;
        Renderer3D& operator=(const Renderer3D&) = delete;

        bool Init(RenderContext* context, const std::wstring& shaderPath);
        void Shutdown();

        void BeginScene(const Camera3D& camera);

        void DrawMesh(const Mesh& mesh,
            const DirectX::XMMATRIX& worldMatrix);

        // Convenience — draw at position with rotation and scale
        void DrawMesh(const Mesh& mesh,
            float x, float y, float z,
            float rotX = 0, float rotY = 0, float rotZ = 0,
            float scaleX = 1, float scaleY = 1, float scaleZ = 1);

        void OnResize(float aspectRatio);

    private:
        RenderContext* m_context = nullptr;
        Shader                      m_shader;

        ComPtr<ID3D11Buffer>        m_constantBuffer;
        ComPtr<ID3D11InputLayout>   m_inputLayout;
        ComPtr<ID3D11RasterizerState>   m_rasterizerState;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;

        DirectX::XMMATRIX           m_viewProjection;
    };
}

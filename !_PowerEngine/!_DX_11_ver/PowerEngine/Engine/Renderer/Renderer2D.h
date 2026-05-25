#pragma once
#include "RenderContext.h"
#include "Shader.h"
#include <DirectXMath.h>

namespace Engine
{
    struct Vertex2D
    {
        float x, y, z;
        float r, g, b, a;
    };

    class Renderer2D
    {
    public:
        Renderer2D() = default;
        ~Renderer2D() = default;

        bool Init(RenderContext* context, const std::wstring& shaderPath);
        void Shutdown();

        void DrawQuad(float x, float y, float w, float h,
            float r, float g, float b, float a = 1.0f);

        void Flush();

        void OnResize(int newWidth, int newHeight);

    private:
        void SetupOrthographicMatrix(int screenW, int screenH);

        RenderContext* m_context = nullptr;
        Shader                          m_shader;

        ComPtr<ID3D11Buffer>            m_vertexBuffer;
        ComPtr<ID3D11Buffer>            m_indexBuffer;
        ComPtr<ID3D11InputLayout>       m_inputLayout;
        ComPtr<ID3D11Buffer>            m_constantBuffer;
        ComPtr<ID3D11RasterizerState>   m_rasterizerState;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;

        DirectX::XMFLOAT4X4             m_orthoMatrix;
    };
}

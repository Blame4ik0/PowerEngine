#pragma once
#include "RenderContext.h"
#include "Shader.h"

namespace Engine
{
    class Renderer2D
    {
    public:
        Renderer2D() = default;
        ~Renderer2D() = default;

        Renderer2D(const Renderer2D&) = delete;
        Renderer2D& operator=(const Renderer2D&) = delete;

        bool Init(RenderContext* context, const std::wstring& shaderPath);
        void Shutdown();

        void DrawTriangle();

    private:
        RenderContext* m_context = nullptr;
        Shader                      m_shader;

        ComPtr<ID3D11Buffer>        m_vertexBuffer;
        ComPtr<ID3D11InputLayout>   m_inputLayout;
        ComPtr<ID3D11RasterizerState>   m_rasterizerState;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;
    };
}

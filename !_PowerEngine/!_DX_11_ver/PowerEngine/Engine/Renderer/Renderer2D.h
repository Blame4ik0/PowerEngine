#pragma once
#include "RenderContext.h"
#include "Shader.h"
#include "Camera2D.h"
#include "Texture2D.h"
#include "Font.h"
#include <memory>
#include <DirectXMath.h>
#include <array>

namespace Engine
{
    struct Vertex2D
    {
        float x, y;
        float r, g, b, a;
        float u, v;
    };

    class Renderer2D
    {
    public:
        static constexpr int MaxVerticesPerBatch = 6000;

        Renderer2D() = default;
        ~Renderer2D() = default;

        Renderer2D(const Renderer2D&) = delete;
        Renderer2D& operator=(const Renderer2D&) = delete;

        bool Init(RenderContext* context, const std::wstring& shaderPath);
        void Shutdown();

        void BeginScene(const Camera2D& camera);
        void BeginScreenSpace();
        void Flush();
        void OnResize(int width, int height);

        // Solid color quad
        void DrawQuad(float x, float y, float w, float h,
            float r, float g, float b, float a = 1.0f);

        // Per-vertex color quad
        void DrawQuad(Vertex2D topLeft, Vertex2D topRight,
            Vertex2D bottomRight, Vertex2D bottomLeft,
            const Texture2D* texture = nullptr);

        // Polygon (3 vertices)
        void DrawPolygon(Vertex2D v0, Vertex2D v1, Vertex2D v2);

		// Text rendering
        void DrawText(const Font& font, const std::string& text,
            float x, float y,
            float r = 1.0f, float g = 1.0f,
            float b = 1.0f, float a = 1.0f);

        int GetDrawCallCount() const { return m_drawCalls; }
        int GetQuadCount()     const { return m_quadCount; }

		// Textured quad (tint color + alpha)
        void DrawSprite(const Texture2D& texture,
            float x, float y, float w, float h,
            float r = 1.0f, float g = 1.0f,
            float b = 1.0f, float a = 1.0f);

    private:
        RenderContext* m_context = nullptr;
        Shader                                  m_shader;

        ComPtr<ID3D11Buffer>                    m_vertexBuffer;
        ComPtr<ID3D11Buffer>                    m_constantBuffer;
        ComPtr<ID3D11InputLayout>               m_inputLayout;
        ComPtr<ID3D11RasterizerState>           m_rasterizerState;
        ComPtr<ID3D11DepthStencilState>         m_depthStencilState;
        ComPtr<ID3D11SamplerState>      m_sampler;
        ComPtr<ID3D11BlendState>        m_blendState;
        std::shared_ptr<Texture2D>      m_whiteTexture;
        const Texture2D* m_currentTexture = nullptr;

        std::array<Vertex2D, MaxVerticesPerBatch> m_vertices{};
        int                                     m_vertexCount = 0;

        DirectX::XMFLOAT4X4                     m_viewProjection{};

        int m_drawCalls = 0;
        int m_quadCount = 0;
    };
}

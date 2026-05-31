#pragma once
#include "Renderer/RenderContext.h"
#include "Renderer/Shader.h"
#include "Camera3D.h"
#include "Mesh.h"
#include <DirectXMath.h>

namespace Engine
{
    class Renderer3D;

    class Grid
    {
    public:
        Grid() = default;
        ~Grid() = default;

        Grid(const Grid&) = delete;
        Grid& operator=(const Grid&) = delete;

        bool Init(RenderContext* context,
            const std::wstring& gridShaderPath,
            Renderer3D* renderer3D,
            int size = 20, float spacing = 1.0f);

        void Draw(const Camera3D& camera);
        void Shutdown();

    private:
        RenderContext* m_context = nullptr;
        Renderer3D* m_renderer3D = nullptr;
        Shader                      m_shader;

        ComPtr<ID3D11Buffer>            m_vertexBuffer;
        ComPtr<ID3D11Buffer>            m_constantBuffer;
        ComPtr<ID3D11InputLayout>       m_inputLayout;
        ComPtr<ID3D11RasterizerState>   m_rasterizerState;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;

        Mesh m_originSphere;
        int  m_vertexCount = 0;
    };
}

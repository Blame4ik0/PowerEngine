#pragma once
#include <DirectXMath.h>

namespace Engine
{
    class Camera2D
    {
    public:
        Camera2D() = default;

        void SetPosition(float x, float y);
        void SetZoom(float zoom);
        void SetViewSize(float width, float height);

        float GetX()    const { return m_x; }
        float GetY()    const { return m_y; }
        float GetZoom() const { return m_zoom; }

        // Combined view-projection matrix for the shader
        DirectX::XMFLOAT4X4 GetViewProjection() const;

    private:
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_zoom = 1.0f;
        float m_width = 1280.0f;
        float m_height = 720.0f;
    };
}

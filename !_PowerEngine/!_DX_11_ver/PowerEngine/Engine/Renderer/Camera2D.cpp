#include "Camera2D.h"

using namespace DirectX;

namespace Engine
{
    void Camera2D::SetPosition(float x, float y)
    {
        m_x = x;
        m_y = y;
    }

    void Camera2D::SetZoom(float zoom)
    {
        // Clamp zoom to avoid divide-by-zero or inverted world
        m_zoom = zoom < 0.01f ? 0.01f : zoom;
    }

    void Camera2D::SetViewSize(float width, float height)
    {
        m_width = width;
        m_height = height;
    }

    XMFLOAT4X4 Camera2D::GetViewProjection() const
    {
        // Orthographic projection — pixel (0,0) top-left
        XMMATRIX proj = XMMatrixOrthographicOffCenterLH(
            0.0f, m_width,
            m_height, 0.0f,
            0.0f, 1.0f);

        // View matrix — translate by -camera position, scale by zoom
        XMMATRIX view = XMMatrixScaling(m_zoom, m_zoom, 1.0f) *
            XMMatrixTranslation(-m_x * m_zoom, -m_y * m_zoom, 0.0f);

        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, view * proj);
        return result;
    }
}

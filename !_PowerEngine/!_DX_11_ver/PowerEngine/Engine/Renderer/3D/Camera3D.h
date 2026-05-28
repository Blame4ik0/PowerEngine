#pragma once
#include <DirectXMath.h>

namespace Engine
{
    class Camera3D
    {
    public:
        Camera3D();

        // Position and orientation
        void SetPosition(float x, float y, float z);
        void SetRotation(float pitch, float yaw, float roll);

        void Move(float dx, float dy, float dz);
        void Rotate(float dpitch, float dyaw, float droll);

        // Projection settings
        void SetPerspective(float fovDegrees, float aspectRatio,
            float nearPlane, float farPlane);

        // Getters
        DirectX::XMFLOAT3 GetPosition() const { return m_position; }
        float GetPitch() const { return m_pitch; }
        float GetYaw()   const { return m_yaw; }

        // Matrices
        DirectX::XMMATRIX GetViewMatrix()       const;
        DirectX::XMMATRIX GetProjectionMatrix() const;
        DirectX::XMMATRIX GetViewProjection()   const;

        // Forward/right/up vectors — useful for movement
        DirectX::XMFLOAT3 GetForward() const;
        DirectX::XMFLOAT3 GetRight()   const;
        DirectX::XMFLOAT3 GetUp()      const;

    private:
        DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, -5.0f };
        float m_pitch = 0.0f;  // up/down rotation in radians
        float m_yaw = 0.0f;  // left/right rotation in radians
        float m_roll = 0.0f;

        float m_fov = 45.0f;
        float m_aspectRatio = 16.0f / 9.0f;
        float m_nearPlane = 0.1f;
        float m_farPlane = 1000.0f;
    };
}

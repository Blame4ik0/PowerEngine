#include "Camera3D.h"

using namespace DirectX;

namespace Engine
{
    Camera3D::Camera3D()
    {
        SetPerspective(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    }

    void Camera3D::SetPosition(float x, float y, float z)
    {
        m_position = { x, y, z };
    }

    void Camera3D::SetRotation(float pitch, float yaw, float roll)
    {
        m_pitch = pitch;
        m_yaw = yaw;
        m_roll = roll;
    }

    void Camera3D::Move(float dx, float dy, float dz)
    {
        XMFLOAT3 fwd = GetForward();
        XMFLOAT3 rgt = GetRight();

        XMVECTOR pos = XMLoadFloat3(&m_position);
        XMVECTOR forward = XMLoadFloat3(&fwd);
        XMVECTOR right = XMLoadFloat3(&rgt);
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        pos = XMVectorAdd(pos, XMVectorScale(forward, dz));
        pos = XMVectorAdd(pos, XMVectorScale(right, dx));
        pos = XMVectorAdd(pos, XMVectorScale(up, dy));

        XMStoreFloat3(&m_position, pos);
    }

    void Camera3D::Rotate(float dpitch, float dyaw, float droll)
    {
        m_pitch += dpitch;
        m_yaw += dyaw;
        m_roll += droll;

        // Clamp pitch to avoid gimbal lock at straight up/down
        constexpr float limit = XM_PIDIV2 - 0.01f;
        if (m_pitch > limit) m_pitch = limit;
        if (m_pitch < -limit) m_pitch = -limit;
    }

    void Camera3D::SetPerspective(float fovDegrees, float aspectRatio,
        float nearPlane, float farPlane)
    {
        m_fov = XMConvertToRadians(fovDegrees);
        m_aspectRatio = aspectRatio;
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
    }

    XMMATRIX Camera3D::GetViewMatrix() const
    {
        // Build rotation matrix from pitch and yaw
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);

        // Default forward is +Z, transform by rotation
        XMVECTOR forward = XMVector3TransformCoord(
            XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);

        XMVECTOR up = XMVector3TransformCoord(
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotation);

        XMVECTOR pos = XMLoadFloat3(&m_position);
        XMVECTOR target = XMVectorAdd(pos, forward);

        return XMMatrixLookAtLH(pos, target, up);
    }

    XMMATRIX Camera3D::GetProjectionMatrix() const
    {
        return XMMatrixPerspectiveFovLH(
            m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
    }

    XMMATRIX Camera3D::GetViewProjection() const
    {
        return GetViewMatrix() * GetProjectionMatrix();
    }

    XMFLOAT3 Camera3D::GetForward() const
    {
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
        XMVECTOR forward = XMVector3TransformCoord(
            XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
        XMFLOAT3 result;
        XMStoreFloat3(&result, forward);
        return result;
    }

    XMFLOAT3 Camera3D::GetRight() const
    {
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
        XMVECTOR right = XMVector3TransformCoord(
            XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotation);
        XMFLOAT3 result;
        XMStoreFloat3(&result, right);
        return result;
    }

    XMFLOAT3 Camera3D::GetUp() const
    {
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
        XMVECTOR up = XMVector3TransformCoord(
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotation);
        XMFLOAT3 result;
        XMStoreFloat3(&result, up);
        return result;
    }
}

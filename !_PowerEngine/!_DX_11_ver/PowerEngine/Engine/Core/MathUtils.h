#pragma once
#include <DirectXMath.h>
#include <cmath>
#include <algorithm>

namespace Engine
{
    // Converts HSV (h: degrees 0-360, s/v: 0-1) to linear RGB (0-1 each).
    inline DirectX::XMFLOAT3 HsvToRgb(float h, float s, float v)
    {
        h = fmodf(h, 360.0f);
        if (h < 0.0f) h += 360.0f;

        float c = v * s;
        float hp = h / 60.0f;
        float x = c * (1.0f - fabsf(fmodf(hp, 2.0f) - 1.0f));
        float m = v - c;

        float r, g, b;
        if (hp < 1) { r = c; g = x; b = 0; }
        else if (hp < 2) { r = x; g = c; b = 0; }
        else if (hp < 3) { r = 0; g = c; b = x; }
        else if (hp < 4) { r = 0; g = x; b = c; }
        else if (hp < 5) { r = x; g = 0; b = c; }
        else { r = c; g = 0; b = x; }

        return { r + m, g + m, b + m };
    }

    // Linear interpolation between two XMFLOAT3 (e.g. colors, positions).
    inline DirectX::XMFLOAT3 Lerp(const DirectX::XMFLOAT3& a,
        const DirectX::XMFLOAT3& b, float t)
    {
        return {
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t
        };
    }

    // Clamp a float between min and max (std::clamp wrapper for convenience).
    inline float Clampf(float v, float lo, float hi)
    {
        return std::max(lo, std::min(hi, v));
    }

    // Degrees <-> radians, thin wrappers around DirectX's own helpers
    // (kept here so callers don't need to remember XMConvertTo* names).
    inline float DegToRad(float deg) { return DirectX::XMConvertToRadians(deg); }
    inline float RadToDeg(float rad) { return DirectX::XMConvertToDegrees(rad); }
}
#pragma once

#include <algorithm>
#include <raylib.h>

namespace Projection {
    constexpr float kFarScale  = 0.55f;   // множитель размера у дальней стенки
    constexpr float kNearScale = 1.45f;   // ... и у ближней

    // Положение по глубине в долях поля: 0 - дальняя стенка, 1 - ближняя
    inline float DepthT(float y, float depth) {
        return depth > 0.0f ? std::clamp(y / depth, 0.0f, 1.0f) : 0.5f;
    }

    inline float DepthScale(float y, float depth) {
        return kFarScale + (kNearScale - kFarScale) * DepthT(y, depth);
    }

    inline Vector2 ToPlane(const Vector3& p)       { return {p.x, -p.z}; }
    inline Vector3 FromPlane(Vector2 plane, float y) { return {plane.x, y, -plane.y}; }
}

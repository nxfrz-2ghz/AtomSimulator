#pragma once

#include <algorithm>
#include <raylib.h>

// Оси симуляции:
//   X - вправо по экрану
//   Y - глубина: y = 0 дальняя стенка, y = Depth ближняя (к зрителю)
//   Z - вверх по экрану
//
// На экране рисуется проекция на плоскость (x, z), а глубина y передаётся
// размером атома: чем ближе к зрителю, тем он крупнее (и ярче).
//
// Мировые координаты Camera2D: (x, -z) - ось Y у raylib направлена вниз,
// поэтому z со знаком минус, и "вверх" в симуляции совпадает с "вверх" на экране.
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

#pragma once

#include <array>
#include <raylib.h>
#include "../atom/atom.h"

class Field {
public:
    enum Axis { X = 0, Y = 1, Z = 2 };

    static constexpr float kCellSize     = 40.0f;
    static constexpr int   kDefaultCols  = 7;   // X
    static constexpr int   kDefaultDepth = 10;    // Y
    static constexpr int   kDefaultRows  = 5;   // Z
    static constexpr int   kMinCells     = 1;
    static constexpr int   kMaxCells     = 100;  // на ось; сетка трёхмерная, так что кубически дорого

    static constexpr float kMaxWallSpeed  = 10.0f;
    static constexpr float kWallSmoothing = 0.2f;
    static constexpr float kMaxWallLead   = 60.0f;

    float Width()  const { return size[X]; }   // X
    float Depth()  const { return size[Y]; }   // Y
    float Height() const { return size[Z]; }   // Z
    Vector3 Size() const { return {size[X], size[Y], size[Z]}; }

    float TargetSize(Axis a) const { return target[a]; }
    float WallSpeed(Axis a)  const { return vel[a]; }

    void Nudge(Axis axis, float deltaSize);
    void NudgeX(float d) { Nudge(X, d); }
    void NudgeY(float d) { Nudge(Y, d); }
    void NudgeZ(float d) { Nudge(Z, d); }
    void Advance(float h);
    void StopWalls();

    bool    Contains(const Vector3& p) const;
    Vector3 ClampInside(Vector3 p) const;
    void    Confine(Atom& a) const;

private:
    std::array<float, 3> size   = {kDefaultCols * kCellSize, kDefaultDepth, kDefaultRows * kCellSize};
    std::array<float, 3> target = {kDefaultCols * kCellSize, kDefaultDepth, kDefaultRows * kCellSize};
    std::array<float, 3> vel    = {0.0f, 0.0f, 0.0f};
};

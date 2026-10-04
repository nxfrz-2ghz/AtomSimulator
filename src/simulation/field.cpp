#include "field.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float kWallRestitution = 1.0f; // wall energy save

    inline void reflect(float& p, float& v, float size, float wallVel) {
        if (p < 0.0f) {
            p = -p;
            v = -v * kWallRestitution;
        } else if (p >= size) {
            p = 2.0f * size - p;
            if (v > wallVel) v = wallVel - (v - wallVel) * kWallRestitution;
        }
        p = std::clamp(p, 0.0f, std::nextafter(size, 0.0f));
    }
}

void Field::Nudge(Axis axis, float deltaSize) {
    const float minS = 1.0f;
    const float maxS = kMaxCells * kCellSize;

    float t = std::clamp(target[axis] + deltaSize, minS, maxS);
    t = std::clamp(t, size[axis] - kMaxWallLead, size[axis] + kMaxWallLead);
    target[axis] = std::clamp(t, minS, maxS);
}

void Field::Advance(float h) {
    for (int a = 0; a < 3; ++a) {
        const float err = target[a] - size[a];
        if (std::fabs(err) < 1e-3f) {
            size[a] = target[a];
            vel[a]  = 0.0f;
            continue;
        }

        const float maxStep = kMaxWallSpeed * h;
        const float step = std::clamp(err, -maxStep, maxStep);
        size[a] += step;
        vel[a]   = step / h;
    }
}

void Field::StopWalls() {
    vel = {0.0f, 0.0f, 0.0f};
    target = size;
}

bool Field::Contains(const Vector3& p) const {
    return p.x >= 0.0f && p.y >= 0.0f && p.z >= 0.0f
        && p.x < size[X] && p.y < size[Y] && p.z < size[Z];
}

Vector3 Field::ClampInside(Vector3 p) const {
    p.x = std::clamp(p.x, 0.0f, std::nextafter(size[X], 0.0f));
    p.y = std::clamp(p.y, 0.0f, std::nextafter(size[Y], 0.0f));
    p.z = std::clamp(p.z, 0.0f, std::nextafter(size[Z], 0.0f));
    return p;
}

void Field::Confine(Atom& a) const {
    reflect(a.position.x, a.velocity.x, size[X], vel[X]);
    reflect(a.position.y, a.velocity.y, size[Y], vel[Y]);
    reflect(a.position.z, a.velocity.z, size[Z], vel[Z]);
}

#pragma once

#include <raylib.h>
#include "../simulation/simulation.h"

class Render {
public:
    void DrawWorld(const Simulation& sim, float zoom, bool showGrid) const;
    void DrawAtoms(const Simulation& sim) const;
    void DrawLabels(const Simulation& sim, const Camera2D& cam) const;
};

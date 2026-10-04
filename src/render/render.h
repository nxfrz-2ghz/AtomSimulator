#pragma once

#include <vector>
#include <raylib.h>
#include "../simulation/simulation.h"

class Render {
public:
    void DrawWorld(const Simulation& sim, float zoom, bool showGrid) const;

    void DrawAtoms(const Simulation& sim, const Camera2D& cam) const;

    void DrawPlacementGhost(const Simulation& sim, const Camera2D& cam,
                            Vector2 worldPlane, float y, unsigned int type) const;

private:
    struct Item { float y; Vector2 screen; unsigned int id; };
    mutable std::vector<Item> items;   // буфер на кадр, чтобы не выделять память каждый раз
};

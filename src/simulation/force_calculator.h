#pragma once

#include <vector>
#include "../atom/atom.h"
#include "cell_grid.h"
#include "field.h"

class ForceCalculator {
public:
    inline static float gravity = 0.0f;

    static double Compute(std::vector<Atom>& atoms, const CellGrid& grid, const Field& field);
};

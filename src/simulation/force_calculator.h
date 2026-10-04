#pragma once

#include <vector>
#include "../atom/atom.h"
#include "cell_grid.h"
#include "field.h"

class ForceCalculator {
public:
    static double Compute(std::vector<Atom>& atoms, const CellGrid& grid, const Field& field);
};

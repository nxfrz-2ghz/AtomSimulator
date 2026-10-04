#include "cell_grid.h"

#include <cmath>

void CellGrid::Resize(const Field& field) {
    const int newX = std::clamp(int(std::ceil(field.Width()  / Field::kCellSize)), 1, Field::kMaxCells);
    const int newY = std::clamp(int(std::ceil(field.Depth()  / Field::kCellSize)), 1, Field::kMaxCells);
    const int newZ = std::clamp(int(std::ceil(field.Height() / Field::kCellSize)), 1, Field::kMaxCells);
    if (newX == nx && newY == ny && newZ == nz) return;

    nx = newX;
    ny = newY;
    nz = newZ;
    cells.assign(size_t(nx) * ny * nz, {});
    usedCells.clear();
}

void CellGrid::Clear() {
    for (int idx : usedCells) cells[idx].clear();
    usedCells.clear();
}

void CellGrid::Rebuild(const std::vector<Atom>& atoms) {
    Clear();
    for (unsigned int i = 0; i < atoms.size(); i++) {
        const int idx = CellOf(atoms[i].position);
        auto& c = cells[idx];
        if (c.empty()) usedCells.push_back(idx);
        c.push_back(i);
    }
}

CellGrid::Coord CellGrid::CoordsOf(const Vector3& p) const {
    return {
        std::clamp(int(p.x / Field::kCellSize), 0, nx - 1),
        std::clamp(int(p.y / Field::kCellSize), 0, ny - 1),
        std::clamp(int(p.z / Field::kCellSize), 0, nz - 1)
    };
}

void CellGrid::Remove(unsigned int id, const Vector3& pos) {
    auto& c = cells[CellOf(pos)];
    for (size_t i = 0; i < c.size(); i++) {
        if (c[i] == id) {
            c[i] = c.back();
            c.pop_back();
            return;
        }
    }
}

void CellGrid::Renumber(unsigned int from, unsigned int to, const Vector3& pos) {
    for (unsigned int& v : cells[CellOf(pos)]) {
        if (v == from) { v = to; break; }
    }
}

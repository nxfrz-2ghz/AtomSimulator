#pragma once

#include <algorithm>
#include <vector>
#include <raylib.h>
#include "../atom/atom.h"
#include "field.h"

class CellGrid {
public:
    struct Coord { int x, y, z; };

    CellGrid() : nx(Field::kDefaultCols), ny(Field::kDefaultDepth), nz(Field::kDefaultRows),
                 cells(size_t(nx) * ny * nz) {}

    int CellsX()    const { return nx; }
    int CellsY()    const { return ny; }
    int CellsZ()    const { return nz; }
    int CellCount() const { return nx * ny * nz; }
    const std::vector<unsigned int>& Cell(int index) const { return cells[index]; }

    void Resize(const Field& field);
    void Clear();
    void Rebuild(const std::vector<Atom>& atoms);

    int   Index(int cx, int cy, int cz) const { return (cz * ny + cy) * nx + cx; }
    Coord CoordsOf(const Vector3& p) const;   // координаты ячейки (с зажимом в границы сетки)
    int   CellOf(const Vector3& p) const { const Coord c = CoordsOf(p); return Index(c.x, c.y, c.z); }

    void Remove(unsigned int id, const Vector3& pos);
    void Renumber(unsigned int from, unsigned int to, const Vector3& pos);

    // Обход атомов в ячейке c и в 26 соседних
    template <class F>
    void ForEachNeighbor(Coord c, F&& f) const {
        for (int dz = -1; dz <= 1; ++dz) {
            const int z = c.z + dz;
            if (z < 0 || z >= nz) continue;
            for (int dy = -1; dy <= 1; ++dy) {
                const int y = c.y + dy;
                if (y < 0 || y >= ny) continue;
                for (int dx = -1; dx <= 1; ++dx) {
                    const int x = c.x + dx;
                    if (x < 0 || x >= nx) continue;
                    for (unsigned int id : cells[Index(x, y, z)]) f(id);
                }
            }
        }
    }

    template <class P>
    bool AnyNeighbor(Coord c, P&& pred) const {
        for (int dz = -1; dz <= 1; ++dz) {
            const int z = c.z + dz;
            if (z < 0 || z >= nz) continue;
            for (int dy = -1; dy <= 1; ++dy) {
                const int y = c.y + dy;
                if (y < 0 || y >= ny) continue;
                for (int dx = -1; dx <= 1; ++dx) {
                    const int x = c.x + dx;
                    if (x < 0 || x >= nx) continue;
                    for (unsigned int id : cells[Index(x, y, z)]) {
                        if (pred(id)) return true;
                    }
                }
            }
        }
        return false;
    }

private:
    int nx, ny, nz;
    std::vector<std::vector<unsigned int>> cells;
    std::vector<int> usedCells;   // непустые ячейки: Clear() не гоняется по всей (кубической) сетке
};

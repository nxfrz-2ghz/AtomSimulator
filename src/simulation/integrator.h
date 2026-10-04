#pragma once

#include <vector>
#include "../atom/atom.h"
#include "field.h"

class Integrator {
public:
    // Половина kick по старым силам, затем drift и отражение от стенок
    static void FirstHalf(std::vector<Atom>& atoms, float h, const Field& field, int skipId);
    // Вторая половина kick по новым силам
    static void SecondHalf(std::vector<Atom>& atoms, float h, int skipId);

    static double KineticEnergy(const std::vector<Atom>& atoms, int skipId = -1);

    static void ScaleVelocities(std::vector<Atom>& atoms, float speedScale, int skipId);
    static void SeedVelocities(std::vector<Atom>& atoms, float speed, int skipId);
};

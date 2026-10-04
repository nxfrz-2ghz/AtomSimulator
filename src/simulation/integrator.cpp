#include "integrator.h"

#include <cmath>
#include <random>

void Integrator::FirstHalf(std::vector<Atom>& atoms, float h, const Field& field, int skipId) {
    const int   n     = static_cast<int>(atoms.size());
    const float halfH = 0.5f * h;

    // first half velocity changes before move
    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        if (i == skipId) continue;   // атом под мышью двигает курсор, а не физика
        Atom& a = atoms[i];
        // 1 / atom.mass - inverted mass чтоб не делить каждый раз
        const float invM = 1.0f / atomTypes[a.type].mass;

        a.velocity.x += a.force.x * invM * halfH;
        a.velocity.y += a.force.y * invM * halfH;
        a.velocity.z += a.force.z * invM * halfH;

        a.position.x += a.velocity.x * h;
        a.position.y += a.velocity.y * h;
        a.position.z += a.velocity.z * h;

        field.Confine(a);
    }
}

void Integrator::SecondHalf(std::vector<Atom>& atoms, float h, int skipId) {
    const int   n     = static_cast<int>(atoms.size());
    const float halfH = 0.5f * h;

    // second half velocity changes after move
    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        if (i == skipId) continue;
        Atom& a = atoms[i];
        const float invM = 1.0f / atomTypes[a.type].mass;
        a.velocity.x += a.force.x * invM * halfH;
        a.velocity.y += a.force.y * invM * halfH;
        a.velocity.z += a.force.z * invM * halfH;
    }
}

double Integrator::KineticEnergy(const std::vector<Atom>& atoms, int skipId) {
    const int n = static_cast<int>(atoms.size());
    double ke = 0.0;

    #pragma omp parallel for reduction(+:ke)
    for (int i = 0; i < n; i++) {
        if (i == skipId) continue;
        // E = mv^2 / 2
        const Atom& a = atoms[i];
        const double v2 = double(a.velocity.x) * a.velocity.x
                        + double(a.velocity.y) * a.velocity.y
                        + double(a.velocity.z) * a.velocity.z;
        ke += 0.5 * atomTypes[a.type].mass * v2;
    }
    return ke;
}

void Integrator::ScaleVelocities(std::vector<Atom>& atoms, float speedScale, int skipId) {
    const int n = static_cast<int>(atoms.size());

    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        if (i == skipId) continue;   // атом под мышью двигает курсор
        atoms[i].velocity.x *= speedScale;
        atoms[i].velocity.y *= speedScale;
        atoms[i].velocity.z *= speedScale;
    }
}

void Integrator::SeedVelocities(std::vector<Atom>& atoms, float speed, int skipId) {
    static std::mt19937 rng{std::random_device{}()};
    std::normal_distribution<float> gauss(0.0f, 1.0f);

    const int n = static_cast<int>(atoms.size());
    for (int i = 0; i < n; ++i) {
        if (i == skipId) continue;
        Vector3 d = {gauss(rng), gauss(rng), gauss(rng)};
        const float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        if (len < 1e-6f) continue;
        // одинаковая скорость -> энергия пропорциональна массе, поэтому делим на sqrt(m): одинаковая энергия на атом
        const float k = speed / (len * std::sqrt(atomTypes[atoms[i].type].mass));
        atoms[i].velocity = {d.x * k, d.y * k, d.z * k};
    }
}

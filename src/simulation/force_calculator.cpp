#include "force_calculator.h"

#include <algorithm>
#include <cmath>

namespace {
    // Радиус обрезки LJ в виде множителя для sigma
    constexpr float kCutoffRatio = 2.5f;
    // Ниже этого расстояния (в sigma) сила не растёт дальше - страховка от взрыва
    constexpr float kMinRatio = 0.7f;

    // Константы "shifted-force" LJ: и сила, и потенциал плавно приходят в 0 на rc.
    // Без этого на границе обрезки сила скачет, и энергия "дышит".
    constexpr float kInvRc   = 1.0f / kCutoffRatio;
    constexpr float kInvRc6  = kInvRc * kInvRc * kInvRc * kInvRc * kInvRc * kInvRc;
    constexpr float kInvRc12 = kInvRc6 * kInvRc6;
    constexpr float kUc = 4.0f * (kInvRc12 - kInvRc6);                    // U(rc)/eps
    constexpr float kFc = 24.0f * (2.0f * kInvRc12 - kInvRc6) * kInvRc;   // F(rc)/(eps/sigma)

    // Мягкая стенка: отталкивающая часть LJ (WCA). Атом не доходит до стенки, а отталкивается от неё
    constexpr float kWallSigmaRatio = 0.5f;       // sigma стенки = 0.5 * sigma атома
    constexpr float kWallEpsScale   = 4.0f;       // жёсткость стенки относительно epsilon атома
    constexpr float kWallCutRatio   = 1.122462f;  // 2^(1/6): дальше этого силы нет

    // d - расстояние от атома до стенки. Возвращает силу (в сторону от стенки), u добавляет в энергию
    inline float wallRepulsion(float d, float sigma, float eps, double& u) {
        const float sw = kWallSigmaRatio * sigma;
        if (d >= kWallCutRatio * sw) return 0.0f;

        const float rEff = std::max(d, kMinRatio * sw);
        const float s    = sw / rEff;
        const float s2   = s * s;
        const float s6   = s2 * s2 * s2;
        const float s12  = s6 * s6;
        const float e    = kWallEpsScale * eps;

        const float f = 24.0f * e * (2.0f * s12 - s6) / rEff;
        u += 4.0f * e * (s12 - s6) + e + f * (rEff - d);
        return f;
    }

    // Взаимодействие атома A с атомом B: добавляет силу на A и половину энергии пары
    inline void pairInteraction(const Atom& a, const Atom& b, Vector3& force, double& peA) {
        const AtomData& dA = atomTypes[a.type];
        const AtomData& dB = atomTypes[b.type];

        const float ddx = a.position.x - b.position.x;
        const float ddy = a.position.y - b.position.y;
        const float ddz = a.position.z - b.position.z;
        const float r2  = ddx * ddx + ddy * ddy + ddz * ddz;

        // global sigma, epsilon and CutoffRadius
        const float sigma   = 0.5f * (dA.sigma + dB.sigma);
        const float epsilon = sqrtf(dA.epsilon * dB.epsilon);
        const float rc = kCutoffRatio * sigma;
        if (r2 >= rc * rc || r2 < 1e-8f) return;

        const float r    = sqrtf(r2);
        // ограничение силы при экстремальном сближении
        const float rEff = std::max(r, kMinRatio * sigma);

        // Расчет сил Леннарда-Джонса (F = -dU/dr)
        const float s   = sigma / rEff;
        const float s2  = s * s;
        const float s6  = s2 * s2 * s2;
        const float s12 = s6 * s6;
        const float epsOverSigma = epsilon / sigma;

        // 24 * epsilon * (2 * (sigma/r)^12 - (sigma/r)^6) / r
        const float fMag = 24.0f * epsilon * (2.0f * s12 - s6) / rEff
                         - kFc * epsOverSigma;

        // potential power
        const float u = 4.0f * epsilon * (s12 - s6)
                      - kUc * epsilon
                      + kFc * epsOverSigma * (rEff - rc);

        const float k = fMag / r;
        force.x += ddx * k;
        force.y += ddy * k;
        force.z += ddz * k;

        // each pair counts twice
        peA += 0.5 * u;
    }
}

double ForceCalculator::Compute(std::vector<Atom>& atoms, const CellGrid& grid, const Field& field) {
    const int     n    = static_cast<int>(atoms.size());
    const Vector3 size = field.Size();
    double pe = 0.0;

    // Идём по атомам, а не по ячейкам: сетка трёхмерная и может быть огромной и почти пустой
    #pragma omp parallel for schedule(dynamic, 64) reduction(+:pe)
    for (int idA = 0; idA < n; idA++) {
        const Atom& atomA = atoms[idA];
        const Vector3 posA = atomA.position;
        const AtomData& dA = atomTypes[atomA.type];
        const CellGrid::Coord cell = grid.CoordsOf(posA);

        Vector3 force = {0.0f, 0.0f, 0.0f};
        double peA = 0.0;

        // interaction with atoms in its and neighbor cells
        grid.ForEachNeighbor(cell, [&](unsigned int idB) {
            if (idA == int(idB)) return;
            pairInteraction(atomA, atoms[idB], force, peA);
        });

        // soft walls (6 штук)
        double peWall = 0.0;
        force.x += wallRepulsion(posA.x,          dA.sigma, dA.epsilon, peWall);
        force.x -= wallRepulsion(size.x - posA.x, dA.sigma, dA.epsilon, peWall);
        force.y += wallRepulsion(posA.y,          dA.sigma, dA.epsilon, peWall);
        force.y -= wallRepulsion(size.y - posA.y, dA.sigma, dA.epsilon, peWall);
        force.z += wallRepulsion(posA.z,          dA.sigma, dA.epsilon, peWall);
        force.z -= wallRepulsion(size.z - posA.z, dA.sigma, dA.epsilon, peWall);
        peA += peWall;

        atoms[idA].force = force;
        pe += peA;
    }
    return pe;
}

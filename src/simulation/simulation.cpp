#include "simulation.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <raymath.h>

namespace {
    // Радиус обрезки LJ в виде множителя для sigma
    constexpr float kCutoffRatio = 2.5f;
    // Ниже этого расстояния (в sigma) сила не растёт дальше - страховка от взрыва
    constexpr float kMinRatio = 0.7f;
    // Нельзя ставить атом ближе чем kPlaceMinRatio * sigma к другому (0 - отключить)
    constexpr float kPlaceMinRatio = 1.0f;
    // 1.0 - упругие стенки (энергия сохраняется), <1 - стенки поглощают энергию
    constexpr float kWallRestitution = 1.0f;
    // Максимальная скорость броска (ед. мира / сим. сек). При шаге 0.01 это 0.3 ед. за шаг -
    // больше нельзя, иначе атом пролетает сквозь других и энергия взрывается.
    constexpr float kMaxThrowSpeed = 30.0f;
    // Постоянная сглаживания скорости курсора (реальные секунды)
    constexpr float kGrabSmoothing = 50.0f;

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

    // Нижняя стенка (0) неподвижна, верхняя (size) едет со скоростью wallVel.
    // Упругий отскок от движущейся стенки: v' = 2*u - v, поэтому сжатие разгоняет атомы.
    inline void reflect(float& p, float& v, float size, float wallVel) {
        if (p < 0.0f) {
            p = -p;
            v = -v * kWallRestitution;
        } else if (p >= size) {
            p = 2.0f * size - p;
            if (v > wallVel) v = wallVel - (v - wallVel) * kWallRestitution;
        }
        // Только страховка от выхода за границу (без "зазора", иначе атом залипает у стенки)
        p = std::clamp(p, 0.0f, std::nextafter(size, 0.0f));
    }
}


void Simulation::Reset() {
    atoms.clear();
    for (auto& c : cells) c.clear();
    grabbedId = -1;
    grabVelocity = {0.0f, 0.0f};

    kinetic = potential = 0.0;
    simTime = 0.0;
    timeAccumulator = 0.0;
    limited = false;
    timeScale = 1.0f;

    wallVel = 0.0f;
    targetWidth = width;
}

void Simulation::NudgeField(float deltaWidth) {
    const float minW = kMinCols * kCellSize;
    const float maxW = kMaxCols * kCellSize;
    targetWidth = std::clamp(targetWidth + deltaWidth, minW, maxW);
    targetWidth = std::clamp(targetWidth, width - kMaxWallLead, width + kMaxWallLead);
    targetWidth = std::clamp(targetWidth, minW, maxW);
}

void Simulation::advanceWalls(float h) {
    constexpr float kGain = 1.0f / (4.0f * kWallSmoothing);

    const float err = targetWidth - width;
    if (std::fabs(err) < 1e-3f && std::fabs(wallVel) < 1e-3f) {
        width = targetWidth;
        wallVel = 0.0f;
        return;
    }

    const float desired = std::clamp(err * kGain, -kMaxWallSpeed, kMaxWallSpeed);
    wallVel += (desired - wallVel) * (1.0f - std::exp(-h / kWallSmoothing));

    float next = width + wallVel * h;
    if ((next - targetWidth) * (width - targetWidth) < 0.0f) {   // страховка от перелёта
        next = targetWidth;
        wallVel = 0.0f;
    }
    width = next;
}

void Simulation::updateGrid() {
    const int newCols = std::clamp(int(std::ceil(Width()  / kCellSize)), 1, kMaxCols);
    const int newRows = std::clamp(int(std::ceil(Height() / kCellSize)), 1, kMaxCols);
    if (newCols == cols && newRows == rows) return;

    cols = newCols;
    rows = newRows;
    cells.assign(size_t(cols) * rows, {});
}

void Simulation::SetTimeScale(float s) {
    timeScale = std::clamp(s, kMinTimeScale, kMaxTimeScale);
}

void Simulation::Update(float realDt) {
    limited = false;
    if (paused) return;

    // Ограничение dt для длинных кадров (перетаскивание окна и т.п.)
    realDt = std::min(realDt, 0.1f);
    timeAccumulator += double(realDt) * timeScale;

    int steps = 0;
    while (timeAccumulator >= kFixedStep) {
        if (steps >= kMaxStepsPerFrame) {
            timeAccumulator = 0.0;
            limited = true;
            break;
        }
        step(kFixedStep);
        steps++;
    }
}

void Simulation::StepOnce() {
    step(kFixedStep);
}


void Simulation::step(float h) {
    const int n = static_cast<int>(atoms.size());
    simTime += h;

    advanceWalls(h);
    updateGrid();
    if (n == 0) return;

	// Velocity Verlet Integrator

    const float halfH = 0.5f * h;

	// first half velocity changes before move
    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        if (i == grabbedId) continue;   // атом под мышью двигает курсор, а не физика
        Atom& a = atoms[i];
        // 1 / atom.mass - inverted mass чтоб не делить каждый раз
        const float invM = 1.0f / atomTypes[a.type].mass;

        a.velocity.x += a.force.x * invM * halfH;
        a.velocity.y += a.force.y * invM * halfH;

        a.position.x += a.velocity.x * h;
        a.position.y += a.velocity.y * h;

        reflect(a.position.x, a.velocity.x, Width(), wallVel);
        reflect(a.position.y, a.velocity.y, Height(), wallVel * kAspect);
    }

    // Атом под мышью физика не двигает - подтягиваем его, если стенка его обогнала
    if (grabbedId >= 0) {
        Atom& g = atoms[grabbedId];
        g.position.x = std::clamp(g.position.x, 0.0f, std::nextafter(Width(), 0.0f));
        g.position.y = std::clamp(g.position.y, 0.0f, std::nextafter(Height(), 0.0f));
    }

    rebuildCells();
	// recount forces in new position
    computeForces();

	// second half velocity changes after move
    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        if (i == grabbedId) continue;
        Atom& a = atoms[i];
        const float invM = 1.0f / atomTypes[a.type].mass;
        a.velocity.x += a.force.x * invM * halfH;
        a.velocity.y += a.force.y * invM * halfH;
    }

    computeKinetic();
}

void Simulation::computeForces() {
    const int totalCells = cols * rows;
    double pe = 0.0;

    #pragma omp parallel for schedule(dynamic) reduction(+:pe)
    // for every cell (multithread)
    for (int cellId = 0; cellId < totalCells; cellId++) {
        const auto& currentCell = cells[cellId];
        if (currentCell.empty()) continue;

        const int cy = cellId / cols;
        const int cx = cellId % cols;

		// for every atom
        for (unsigned int idA : currentCell) {
            const Vector2 posA = atoms[idA].position;
            const AtomData& dA = atomTypes[atoms[idA].type];
            Vector2 force = {0.0f, 0.0f};
            double peA = 0.0;

            // interaction with atoms in its and neighbor cells
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const int nx = cx + dx;
                    const int ny = cy + dy;
                    if (nx < 0 || ny < 0 || nx >= cols || ny >= rows) continue;

                    for (unsigned int idB : cells[ny * cols + nx]) {
                        if (idA == idB) continue;

                        const Vector2 posB = atoms[idB].position;
                        const float ddx = posA.x - posB.x;
                        const float ddy = posA.y - posB.y;
                        const float r2  = ddx * ddx + ddy * ddy;

                        const AtomData& dB = atomTypes[atoms[idB].type];
                        // global sigma, epsilon and CutoffRadius
                        const float sigma   = 0.5f * (dA.sigma + dB.sigma);
                        const float epsilon = sqrtf(dA.epsilon * dB.epsilon);
                        const float rc = kCutoffRatio * sigma;
                        if (r2 >= rc * rc || r2 < 1e-8f) continue;

                        const float r    = sqrtf(r2);
                        // ограничение силы при экстремальном сближении
                        const float rEff = std::max(r, kMinRatio * sigma);

                        // Расчет сил Леннарда-Джонса (F = -dU/dr)
                        const float s   = sigma / rEff;
                        const float s2  = s * s;
                        const float s6  = s2 * s2 * s2;
                        const float s12 = s6 * s6;
                        const float epsOverSigma = epsilon / sigma;

                        // 24 * epsilon * (2 * (sigma/r)^12 - (sigma/r)^6) / r^2
                        const float fMag = 24.0f * epsilon * (2.0f * s12 - s6) / rEff
                                         - kFc * epsOverSigma;

                        // potential power
                        const float u = 4.0f * epsilon * (s12 - s6)
                                      - kUc * epsilon
                                      + kFc * epsOverSigma * (rEff - rc);

                        const float k = fMag / r;
                        force.x += ddx * k;
                        force.y += ddy * k;

                        // each pair counts twice
                        peA += 0.5 * u;
                    }
                }
            }

            // Мягкие стенки: четыре отталкивающих "стены"
            double peWall = 0.0;
            force.x += wallRepulsion(posA.x,                 dA.sigma, dA.epsilon, peWall);
            force.x -= wallRepulsion(Width()  - posA.x,      dA.sigma, dA.epsilon, peWall);
            force.y += wallRepulsion(posA.y,                 dA.sigma, dA.epsilon, peWall);
            force.y -= wallRepulsion(Height() - posA.y,      dA.sigma, dA.epsilon, peWall);
            peA += peWall;

            atoms[idA].force = force;
            pe += peA;
        }
    }

    potential = energyCount ? pe : 0.0;
}

void Simulation::computeKinetic() {
    if (!energyCount) { kinetic = 0.0; return; }

    const int n = static_cast<int>(atoms.size());
    double ke = 0.0;

    #pragma omp parallel for reduction(+:ke)
    for (int i = 0; i < n; i++) {
    	// E = mv^2 / 2
        const Atom& a = atoms[i];
        const double v2 = double(a.velocity.x) * a.velocity.x
                        + double(a.velocity.y) * a.velocity.y;
        // each pair counts twice
        ke += 0.5 * atomTypes[a.type].mass * v2;
    }
    kinetic = ke;
}

void Simulation::refreshState() {
    rebuildCells();
    if (atoms.empty()) { kinetic = potential = 0.0; return; }
    computeForces();
    computeKinetic();
}


bool Simulation::overlapsExisting(Vector2 pos, unsigned int type) const {
    if (kPlaceMinRatio <= 0.0f) return false;

    const int cx = int(pos.x / kCellSize);
    const int cy = int(pos.y / kCellSize);

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            const int nx = cx + dx, ny = cy + dy;
            if (nx < 0 || ny < 0 || nx >= cols || ny >= rows) continue;

            for (unsigned int id : cells[ny * cols + nx]) {
                const Atom& a = atoms[id];
                const float sigma = 0.5f * (atomTypes[type].sigma + atomTypes[a.type].sigma);
                const float minDist = kPlaceMinRatio * sigma;
                if (Vector2DistanceSqr(a.position, pos) < minDist * minDist) return true;
            }
        }
    }
    return false;
}

bool Simulation::AddAtom(Vector2 pos, unsigned int type) {
    if (!insideWorld(pos)) return false;
    if (overlapsExisting(pos, type)) return false;

    atoms.push_back(Atom{pos, Vector2{0.0f, 0.0f}, type});
    refreshState();
    return true;
}

bool Simulation::BeginGrab(Vector2 worldPos, float pickRadius) {
    int   best = -1;
    float bestDist2 = FLT_MAX;

    for (size_t i = 0; i < atoms.size(); i++) {
        const float r = std::max(atomTypes[atoms[i].type].radius, pickRadius);
        const float d2 = Vector2DistanceSqr(atoms[i].position, worldPos);
        if (d2 <= r * r && d2 < bestDist2) {
            best = int(i);
            bestDist2 = d2;
        }
    }
    if (best < 0) return false;

    grabbedId = best;
    grabOffset = Vector2Subtract(atoms[best].position, worldPos);  // без "прыжка" к курсору
    grabVelocity = {0.0f, 0.0f};
    atoms[best].velocity = grabVelocity;
    return true;
}

void Simulation::DragTo(Vector2 worldPos, float realDt) {
    if (grabbedId < 0 || realDt <= 0.0f) return;
    Atom& a = atoms[grabbedId];

    Vector2 target = Vector2Add(worldPos, grabOffset);
    target.x = std::clamp(target.x, 0.0f, std::nextafter(Width(), 0.0f));
    target.y = std::clamp(target.y, 0.0f, std::nextafter(Height(), 0.0f));

    // Скорость в единицах СИМУЛЯЦИИ: делим на realDt * timeScale,
    // тогда на любом масштабе времени атом продолжает лететь с той же видимой скоростью
    const float simDt = realDt * timeScale;
    Vector2 inst = {(target.x - a.position.x) / simDt, (target.y - a.position.y) / simDt};

    // Вызываем каждый кадр (и при неподвижной мыши), поэтому если мышь остановили - скорость затухает
    const float k = 1.0f - std::exp(-realDt / kGrabSmoothing);
    grabVelocity.x += (inst.x - grabVelocity.x) * k;
    grabVelocity.y += (inst.y - grabVelocity.y) * k;

    const float speed = Vector2Length(grabVelocity);
    if (speed > kMaxThrowSpeed) grabVelocity = Vector2Scale(grabVelocity, kMaxThrowSpeed / speed);

    a.position = target;
    a.velocity = grabVelocity;

    // Позиция изменилась -> обновляем ячейки (иначе eraseAtom/силы работают по старым)
    if (paused) refreshState(); else rebuildCells();
}

void Simulation::EndGrab() {
    // Скорость уже лежит в atoms[grabbedId].velocity - дальше интегратор подхватит атом сам
    grabbedId = -1;
    grabVelocity = {0.0f, 0.0f};
}

bool Simulation::RemoveAtom(Vector2 pos) {
    if (!insideWorld(pos)) return false;

    const int cx = int(pos.x / kCellSize);
    const int cy = int(pos.y / kCellSize);

    int   best = -1;
    float bestDist2 = FLT_MAX;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int nx = cx + dx, ny = cy + dy;
            if (nx < 0 || ny < 0 || nx >= cols || ny >= rows) continue;

            for (unsigned int id : cells[ny * cols + nx]) {
                const Atom& a = atoms[id];
                float r = atomTypes[a.type].radius;
                float d2 = Vector2DistanceSqr(a.position, pos);
                if (d2 <= r * r && d2 < bestDist2) {
                    best = int(id);
                    bestDist2 = d2;
                }
            }
        }
    }

    if (best < 0) return false;
    eraseAtom(best);
    refreshState();
    return true;
}

bool Simulation::insideWorld(Vector2 p) const {
    return p.x >= 0.0f && p.y >= 0.0f && p.x < Width() && p.y < Height();
}

int Simulation::cellOf(Vector2 p) const {
    int cx = int(p.x / kCellSize);
    int cy = int(p.y / kCellSize);

    if (cx < 0) cx = 0;
    if (cx >= cols) cx = cols - 1;
    if (cy < 0) cy = 0;
    if (cy >= rows) cy = rows - 1;

    return cy * cols + cx;
}

void Simulation::eraseAtom(unsigned int id) {
    auto removeFromCell = [&](int cell, unsigned int value) {
        auto& c = cells[cell];
        for (size_t i = 0; i < c.size(); i++) {
            if (c[i] == value) {
                c[i] = c.back();
                c.pop_back();
                return;
            }
        }
    };
    removeFromCell(cellOf(atoms[id].position), id);

    if (grabbedId == int(id)) EndGrab();
    unsigned int last = atoms.size() - 1;
    if (grabbedId == int(last)) grabbedId = int(id);   // последний атом переезжает на место id
    if (id != last) {
        auto& c = cells[cellOf(atoms[last].position)];
        for (unsigned int& v : c) {
            if (v == last) { v = id; break; }   // в ячейке заменяем старый индекс на новый
        }
        atoms[id] = atoms[last];
    }
    atoms.pop_back();
}

void Simulation::rebuildCells() {
    for (auto& c : cells) c.clear();
    for (unsigned int i = 0; i < atoms.size(); i++) {
        cells[cellOf(atoms[i].position)].push_back(i);
    }
}

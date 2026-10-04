#include "simulation.h"

#include <cfloat>
#include <cmath>
#include <raymath.h>

#include "../utils/projection.h"
#include "force_calculator.h"

namespace {
    constexpr float kPlaceMinRatio = 1.0f;

    constexpr double kSeedThreshold = 1e-6;
    constexpr float  kSeedSpeed     = 0.05f;
}

void Simulation::Reset() {
    atoms.clear();
    grid.Clear();
    grabber.Reset();

    kinetic = potential = 0.0;
    time.Reset();
    field.StopWalls();
}

void Simulation::Update(float realDt) {
    const int steps = time.PlanSteps(realDt);
    for (int i = 0; i < steps; ++i) step(TimeController::kFixedStep);
}

void Simulation::StepOnce() {
    step(TimeController::kFixedStep);
}

void Simulation::step(float h) {
    time.AddSimTime(h);

    field.Advance(h);
    grid.Resize(field);
    if (atoms.empty()) return;

    // Velocity Verlet: kick + drift -> пересчёт сил в новых позициях -> kick
    Integrator::FirstHalf(atoms, h, field, grabber.Id());

    // Атом под мышью физика не двигает - подтягиваем его, если стенка его обогнала
    if (grabber.IsActive()) {
        Atom& g = atoms[grabber.Id()];
        g.position = field.ClampInside(g.position);
    }

    grid.Rebuild(atoms);
    computeForces();

    Integrator::SecondHalf(atoms, h, grabber.Id());

    computeKinetic();
}

void Simulation::ScaleKineticEnergy(float energyFactor) {
    if (atoms.empty() || energyFactor <= 0.0f || energyFactor == 1.0f) return;

    // Грабаемый атом не масштабируем, поэтому и энергию считаем без него
    const double ke = Integrator::KineticEnergy(atoms, grabber.Id());

    if (ke < kSeedThreshold) {
        // Охлаждать уже нечего; при нагреве даём затравку, дальше энергия растёт экспоненциально
        if (energyFactor > 1.0f) Integrator::SeedVelocities(atoms, kSeedSpeed, grabber.Id());
    } else {
        Integrator::ScaleVelocities(atoms, std::sqrt(energyFactor), grabber.Id());
    }

    computeKinetic();   // чтобы значения на экране обновились сразу, в том числе на паузе
}

void Simulation::computeForces() {
    const double pe = ForceCalculator::Compute(atoms, grid, field);
    potential = energyCount ? pe : 0.0;
}

void Simulation::computeKinetic() {
    kinetic = energyCount ? Integrator::KineticEnergy(atoms) : 0.0;
}

void Simulation::refreshState() {
    grid.Rebuild(atoms);
    if (atoms.empty()) { kinetic = potential = 0.0; return; }
    computeForces();
    computeKinetic();
}

bool Simulation::overlapsExisting(const Vector3& pos, unsigned int type) const {
    if (kPlaceMinRatio <= 0.0f) return false;

    return grid.AnyNeighbor(grid.CoordsOf(pos), [&](unsigned int id) {
        const Atom& a = atoms[id];
        const float sigma = 0.5f * (atomTypes[type].sigma + atomTypes[a.type].sigma);
        const float minDist = kPlaceMinRatio * sigma;
        return DistanceSqr(a.position, pos) < minDist * minDist;
    });
}

bool Simulation::AddAtom(Vector3 pos, unsigned int type) {
    if (!field.Contains(pos)) return false;
    if (overlapsExisting(pos, type)) return false;

    atoms.push_back(Atom{pos, Vector3{0.0f, 0.0f, 0.0f}, type, Vector3{0.0f, 0.0f, 0.0f}});
    refreshState();
    return true;
}

// Выбор атома по точке на экране. Экран показывает проекцию на (x, z), так что под курсором
// может оказаться несколько атомов на разной глубине - выигрывает ближайший к зрителю (больший y).
//  1) атомы, в чей нарисованный круг курсор попал по-настоящему: берём самый передний;
//  2) если таких нет, а задан minPickRadius (запас на неточное попадание) - ближайший к курсору.
int Simulation::pickAtom(Vector2 plane, float minPickRadius) const {
    const float depth = field.Depth();

    int   best = -1;
    float bestY = -FLT_MAX;
    for (size_t i = 0; i < atoms.size(); i++) {
        const Atom& a = atoms[i];
        const float r = atomTypes[a.type].radius * Projection::DepthScale(a.position.y, depth);
        if (Vector2DistanceSqr(Projection::ToPlane(a.position), plane) > r * r) continue;
        if (a.position.y > bestY) { best = int(i); bestY = a.position.y; }
    }
    if (best >= 0 || minPickRadius <= 0.0f) return best;

    float bestDist2 = FLT_MAX;
    for (size_t i = 0; i < atoms.size(); i++) {
        const Atom& a = atoms[i];
        const float r = std::max(atomTypes[a.type].radius * Projection::DepthScale(a.position.y, depth),
                                 minPickRadius);
        const float d2 = Vector2DistanceSqr(Projection::ToPlane(a.position), plane);
        if (d2 <= r * r && d2 < bestDist2) { best = int(i); bestDist2 = d2; }
    }
    return best;
}

bool Simulation::RemoveAtom(Vector2 plane) {
    const int best = pickAtom(plane, 0.0f);
    if (best < 0) return false;

    eraseAtom(best);
    refreshState();
    return true;
}

void Simulation::eraseAtom(unsigned int id) {
    grid.Remove(id, atoms[id].position);

    const unsigned int last = atoms.size() - 1;
    grabber.OnAtomRemoved(id, last);

    if (id != last) {
        grid.Renumber(last, id, atoms[last].position);   // в ячейке заменяем старый индекс на новый
        atoms[id] = atoms[last];
    }
    atoms.pop_back();
}

bool Simulation::BeginGrab(Vector2 plane, float pickRadius) {
    const int id = pickAtom(plane, pickRadius);
    if (id < 0) return false;

    grabber.Begin(atoms, id, Projection::FromPlane(plane, atoms[id].position.y));
    return true;
}

void Simulation::DragTo(Vector2 plane, float realDt) {
    if (!grabber.IsActive()) return;

    const float y = atoms[grabber.Id()].position.y;   // глубина при перетаскивании не меняется
    if (!grabber.DragTo(atoms, field, Projection::FromPlane(plane, y), realDt, time.Scale())) return;

    if (time.IsPaused()) refreshState(); else grid.Rebuild(atoms);
}

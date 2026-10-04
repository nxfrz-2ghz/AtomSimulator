#pragma once

#include <vector>
#include <raylib.h>
#include "../atom/atom.h"
#include "integrator.h"
#include "cell_grid.h"
#include "field.h"
#include "grabber.h"
#include "time_controller.h"

// Оси: X - вправо, Y - глубина, Z - вверх (подробнее в utils/projection.h).
class Simulation {
public:
    static constexpr float kCellSize = Field::kCellSize;

    bool energyCount = true;

    // Field / grid
    int   CellsX() const { return grid.CellsX(); }
    int   CellsY() const { return grid.CellsY(); }
    int   CellsZ() const { return grid.CellsZ(); }
    float Width()  const { return field.Width(); }    // X
    float Depth()  const { return field.Depth(); }    // Y
    float Height() const { return field.Height(); }   // Z

    void  NudgeField(Field::Axis axis, float deltaSize) { field.Nudge(axis, deltaSize); }
    float TargetSize(Field::Axis axis) const { return field.TargetSize(axis); }
    float WallSpeed(Field::Axis axis)  const { return field.WallSpeed(axis); }

    void Reset();

    void Update(float realDt);
    void StepOnce();

    // Time
    void   SetPaused(bool p) { time.SetPaused(p); }
    void   TogglePause()     { time.TogglePause(); }
    bool   IsPaused() const  { return time.IsPaused(); }

    void   SetTimeScale(float s) { time.SetScale(s); }
    float  TimeScale() const     { return time.Scale(); }
    bool   SpeedLimited() const  { return time.Limited(); } // компьютер не успевает за выбранной скоростью
    double SimTime() const       { return time.SimTime(); }

    // Energy
    double KineticEnergy()   const { return kinetic; }
    double PotentialEnergy() const { return potential; }
    double TotalEnergy()     const { return kinetic + potential; }
    void ScaleKineticEnergy(float energyFactor);

    // Atoms
    bool AddAtom(Vector3 pos, unsigned int type);
    bool RemoveAtom(Vector2 plane);
    const std::vector<Atom>& Atoms() const { return atoms; }

    // Grab (перетаскивание в плоскости экрана, глубина атома сохраняется)
    bool BeginGrab(Vector2 plane, float pickRadius);
    void DragTo(Vector2 plane, float realDt);
    void EndGrab()                 { grabber.End(); }
    bool IsGrabbing() const        { return grabber.IsActive(); }

private:
    std::vector<Atom> atoms;

    Field          field;
    CellGrid       grid;
    TimeController time;
    Grabber        grabber;

    double kinetic   = 0.0;
    double potential = 0.0;

    void step(float h);
    void computeForces();
    void computeKinetic();
    void refreshState();

    int  pickAtom(Vector2 plane, float minPickRadius) const;
    bool overlapsExisting(const Vector3& pos, unsigned int type) const;
    void eraseAtom(unsigned int id);
};

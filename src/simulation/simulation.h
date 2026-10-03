#pragma once

#include <vector>
#include "../atom/atom.h"

class Simulation {
public:
	// Grid
    static constexpr float kCellSize    = 40.0f;
    static constexpr int   kDefaultCols = 20;
    static constexpr int   kDefaultRows = 11;
    static constexpr int   kMinCols     = 1;
    static constexpr int   kMaxCols     = 200;
    // Пропорции поля фиксированы: высота = ширина * kAspect
    static constexpr float kAspect      = float(kDefaultRows) / float(kDefaultCols);

    static constexpr float kMaxWallSpeed  = 10.0f;
    static constexpr float kWallSmoothing = 0.2f;
    static constexpr float kMaxWallLead   = 60.0f;

    // Steps and Time
    static constexpr float kFixedStep        = 0.01f;   // шаг интегрирования (ед. времени симуляции)
    static constexpr int   kMaxStepsPerFrame = 100;
    static constexpr float kMinTimeScale     = 1.0f / 16.0f;
    static constexpr float kMaxTimeScale     = 128.0f;

    bool energyCount = true;

    int   Cols()   const { return cols; }
    int   Rows()   const { return rows; }
    float Width()  const { return width; }
    float Height() const { return width * kAspect; }

    void  NudgeField(float deltaWidth);
    float TargetWidth() const { return targetWidth; }
    float WallSpeed()   const { return wallVel; }

    void Reset();

    void Update(float realDt);
    void StepOnce();

    void  SetPaused(bool p) { paused = p; }
    void  TogglePause()     { paused = !paused; }
    bool  IsPaused() const  { return paused; }

    void  SetTimeScale(float s);
    float TimeScale() const { return timeScale; }
    bool  SpeedLimited() const { return limited; } // компьютер не успевает за выбранной скоростью
    double SimTime() const  { return simTime; }

    double KineticEnergy()   const { return kinetic; }
    double PotentialEnergy() const { return potential; }
    double TotalEnergy()     const { return kinetic + potential; }

    bool AddAtom(Vector2 pos, unsigned int type);

    // Grab
    bool BeginGrab(Vector2 worldPos, float pickRadius);
    void DragTo(Vector2 worldPos, float realDt);
    void EndGrab();
    bool IsGrabbing() const { return grabbedId >= 0; }
    bool RemoveAtom(Vector2 pos);

    const std::vector<Atom>& Atoms() const { return atoms; }

private:
    int cols = kDefaultCols;
    int rows = kDefaultRows;

    // Поле
    float width       = kDefaultCols * kCellSize;
    float targetWidth = kDefaultCols * kCellSize;
    float wallVel     = 0.0f;

    std::vector<Atom> atoms;
    std::vector<std::vector<unsigned int>> cells =
        std::vector<std::vector<unsigned int>>(kDefaultCols * kDefaultRows);

    // Time
    bool   paused      = false;
    float  timeScale   = 1.0f;
    bool   limited     = false;
    double timeAccumulator = 0.0;
    double simTime     = 0.0;

    // Grab
    int     grabbedId   = -1;
    Vector2 grabOffset  = {0.0f, 0.0f};   // atom.pos - курсор в момент захвата
    Vector2 grabVelocity = {0.0f, 0.0f};  // сглаженная скорость (ед. мира / сим. сек)

    double kinetic   = 0.0;
    double potential = 0.0;

    bool insideWorld(Vector2 p) const;
    int  cellOf(Vector2 p) const;
    void eraseAtom(unsigned int id);
    void rebuildCells();
    void advanceWalls(float h);
    void updateGrid();

    void step(float h);
    void computeForces();
    void computeKinetic();
    void refreshState();
    bool overlapsExisting(Vector2 pos, unsigned int type) const;
};

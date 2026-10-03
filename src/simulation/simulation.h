#pragma once

#include <vector>
#include "../atom/atom.h"

class Simulation {
public:
	// Grid
    static constexpr float kCellSize = 40.0f;
    static constexpr int   kCols     = 40;
    static constexpr int   kRows     = 22;
    static constexpr float kWidth    = kCols * kCellSize;
    static constexpr float kHeight   = kRows * kCellSize;

    // Steps and Time
    static constexpr float kFixedStep        = 0.01f;   // шаг интегрирования (ед. времени симуляции)
    static constexpr int   kMaxStepsPerFrame = 100;
    static constexpr float kMinTimeScale     = 1.0f / 16.0f;
    static constexpr float kMaxTimeScale     = 128.0f;

    bool energyCount = true;

    void Update(float realDt);
    void StepOnce();                       // один шаг (удобно на паузе)

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
    bool RemoveAtom(Vector2 pos);

    const std::vector<Atom>& Atoms() const { return atoms; }

private:
    std::vector<Atom> atoms;
    std::vector<std::vector<unsigned int>> cells =
        std::vector<std::vector<unsigned int>>(kCols * kRows);

    bool   paused      = false;
    float  timeScale   = 1.0f;
    bool   limited     = false;
    double timeAccumulator = 0.0;
    double simTime     = 0.0;

    double kinetic   = 0.0;
    double potential = 0.0;

    static bool insideWorld(Vector2 p);
    static int  cellOf(Vector2 p);
    void eraseAtom(unsigned int id);
    void rebuildCells();

    void step(float h);
    void computeForces();
    void computeKinetic();
    void refreshState();
    bool overlapsExisting(Vector2 pos, unsigned int type) const;
};

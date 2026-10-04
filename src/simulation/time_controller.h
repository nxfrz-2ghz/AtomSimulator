#pragma once

#include <algorithm>

class TimeController {
public:
    static constexpr float kFixedStep        = 0.01f;   // шаг интегрирования (ед. времени симуляции)
    static constexpr int   kMaxStepsPerFrame = 100;
    static constexpr float kMinTimeScale     = 1.0f / 16.0f;
    static constexpr float kMaxTimeScale     = 128.0f;

    void  SetPaused(bool p) { paused = p; }
    void  TogglePause()     { paused = !paused; }
    bool  IsPaused() const  { return paused; }

    void  SetScale(float s) { scale = std::clamp(s, kMinTimeScale, kMaxTimeScale); }
    float Scale() const     { return scale; }

    bool   Limited() const  { return limited; }   // компьютер не успевает за выбранной скоростью
    double SimTime() const  { return simTime; }

    // Сколько фиксированных шагов нужно сделать за этот кадр
    int  PlanSteps(float realDt);
    // Учесть выполненный шаг в общем времени симуляции
    void AddSimTime(float h) { simTime += h; }

    // Сброс времени и масштаба (пауза не трогается)
    void Reset();

private:
    bool   paused      = false;
    float  scale       = 1.0f;
    bool   limited     = false;
    double accumulator = 0.0;
    double simTime     = 0.0;
};

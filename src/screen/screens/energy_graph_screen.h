#pragma once

#include <deque>
#include "../screen.h"

// График полной энергии системы от времени симуляции.
// История копится в фоне (в Update), поэтому график не пустой сразу после открытия.
class EnergyGraphScreen : public Screen {
public:
    const char* Title() const override { return "Energy graph"; }
    Vector2 Size() const override { return {500.0f, 260.0f}; }

    void Update(const ScreenContext& ctx, float dt) override;
    void Draw(const ScreenContext& ctx, Rectangle area) override;

private:
    struct Sample { double time; double energy; };

    static constexpr double kSampleInterval = 0.1;   // в единицах времени симуляции
    static constexpr size_t kMaxSamples     = 600;   // окно = 60 ед. времени симуляции

    std::deque<Sample> samples;
    double lastSampleTime = -1e9;
};

#include "time_controller.h"

int TimeController::PlanSteps(float realDt) {
    limited = false;
    if (paused) return 0;

    // Ограничение dt для длинных кадров (перетаскивание окна и т.п.)
    realDt = std::min(realDt, 0.1f);
    accumulator += double(realDt) * scale;

    int steps = 0;
    while (accumulator >= kFixedStep) {
        if (steps >= kMaxStepsPerFrame) {
            accumulator = 0.0;
            limited = true;
            break;
        }
        accumulator -= kFixedStep;
        steps++;
    }
    return steps;
}

void TimeController::Reset() {
    accumulator = 0.0;
    simTime     = 0.0;
    limited     = false;
    scale       = 1.0f;
}

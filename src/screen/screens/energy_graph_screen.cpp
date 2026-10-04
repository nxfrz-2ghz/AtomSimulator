#include "energy_graph_screen.h"

#include <algorithm>
#include <cmath>

void EnergyGraphScreen::Update(const ScreenContext& ctx, float) {
    const double t = ctx.sim.SimTime();

    // Время откатилось назад - значит был Reset, старая история не нужна
    if (t < lastSampleTime) {
        samples.clear();
        lastSampleTime = -1e9;
    }

    // На паузе simTime не меняется, так что новые точки не добавляются
    if (t - lastSampleTime < kSampleInterval) return;

    samples.push_back({t, ctx.sim.TotalEnergy()});
    lastSampleTime = t;
    if (samples.size() > kMaxSamples) samples.pop_front();
}

void EnergyGraphScreen::Draw(const ScreenContext& ctx, Rectangle area) {
    constexpr int   kFont       = 16;
    constexpr float kLeftMargin = 70.0f;   // место под подписи оси Y
    constexpr float kTopMargin  = 24.0f;   // строка с текущим значением
    constexpr float kBottomMargin = 22.0f; // подписи оси X

    DrawText(TextFormat("Total energy: %.3f", ctx.sim.TotalEnergy()),
             int(area.x), int(area.y), kFont + 2, RAYWHITE);

    const Rectangle plot = {
        area.x + kLeftMargin,
        area.y + kTopMargin,
        area.width  - kLeftMargin,
        area.height - kTopMargin - kBottomMargin
    };
    if (plot.width < 10.0f || plot.height < 10.0f) return;

    DrawRectangleLinesEx(plot, 1.0f, DARKGRAY);
    if (samples.size() < 2) return;

    // Диапазон по Y с запасом, чтобы линия не липла к краям
    double lo = samples.front().energy, hi = lo;
    for (const Sample& s : samples) {
        lo = std::min(lo, s.energy);
        hi = std::max(hi, s.energy);
    }
    double pad = (hi - lo) * 0.1;
    if (pad < 1e-6) pad = std::max(1.0, std::fabs(hi) * 0.01);   // плоская линия
    lo -= pad;
    hi += pad;

    const double t0 = samples.front().time;
    const double t1 = samples.back().time;
    const double dt = std::max(t1 - t0, 1e-9);

    auto toScreen = [&](const Sample& s) {
        return Vector2{
            plot.x + float((s.time - t0) / dt) * plot.width,
            plot.y + plot.height - float((s.energy - lo) / (hi - lo)) * plot.height
        };
    };

    // Горизонтальные линии сетки и подписи оси Y
    constexpr int kDivisions = 4;
    for (int i = 0; i <= kDivisions; i++) {
        const float f = float(i) / kDivisions;
        const float y = plot.y + plot.height * (1.0f - f);
        if (i > 0 && i < kDivisions) DrawLineEx({plot.x, y}, {plot.x + plot.width, y}, 1.0f, Fade(DARKGRAY, 0.5f));

        const char* label = TextFormat("%.2f", lo + (hi - lo) * f);
        DrawText(label, int(plot.x) - MeasureText(label, kFont) - 6, int(y) - kFont / 2, kFont, GRAY);
    }

    // Подписи оси X (время симуляции)
    DrawText(TextFormat("%.1f", t0), int(plot.x), int(plot.y + plot.height + 4), kFont, GRAY);
    const char* tEnd = TextFormat("t = %.1f", t1);
    DrawText(tEnd, int(plot.x + plot.width) - MeasureText(tEnd, kFont),
             int(plot.y + plot.height + 4), kFont, GRAY);

    // Сама кривая
    Vector2 prev = toScreen(samples.front());
    for (size_t i = 1; i < samples.size(); i++) {
        const Vector2 cur = toScreen(samples[i]);
        DrawLineEx(prev, cur, 2.0f, GREEN);
        prev = cur;
    }
}

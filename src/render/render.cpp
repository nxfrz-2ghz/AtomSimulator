#include "render.h"

void Render::DrawWorld(float zoom, bool showGrid) const {
    // толщина делится на zoom, чтобы линии оставались ~1 px при любом масштабе
    const float thin = 1.0f / zoom;

    if (showGrid) {
        Color c = Fade(DARKGRAY, 0.6f);
        for (int i = 1; i < Simulation::kCols; i++) {
            float x = i * Simulation::kCellSize;
            DrawLineEx({x, 0.0f}, {x, Simulation::kHeight}, thin, c);
        }
        for (int j = 1; j < Simulation::kRows; j++) {
            float y = j * Simulation::kCellSize;
            DrawLineEx({0.0f, y}, {Simulation::kWidth, y}, thin, c);
        }
    }

    DrawRectangleLinesEx({0.0f, 0.0f, Simulation::kWidth, Simulation::kHeight},
                         2.0f / zoom, GRAY);
}

void Render::DrawAtoms(const Simulation& sim) const {
    for (const Atom& a : sim.Atoms()) {
        const AtomData& d = atomTypes[a.type];
        DrawCircleV({a.position.x, a.position.y}, d.radius, d.color);
    }
}

void Render::DrawLabels(const Simulation& sim, const Camera2D& cam) const {
    constexpr int fontSize = 14;
    const int sw = GetScreenWidth(), sh = GetScreenHeight();

    for (const Atom& a : sim.Atoms()) {
        const AtomData& d = atomTypes[a.type];

        // если атом на экране слишком мелкий, подпись только мешает
        if (d.radius * cam.zoom < fontSize * 0.6f) continue;

        Vector2 s = GetWorldToScreen2D({a.position.x, a.position.y}, cam);
        if (s.x < -50 || s.y < -50 || s.x > sw + 50 || s.y > sh + 50) continue;

        int w = MeasureText(d.symbol, fontSize);
        DrawText(d.symbol, int(s.x) - w / 2, int(s.y) - fontSize / 2, fontSize, BLACK);
    }
}

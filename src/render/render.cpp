#include "render.h"

#include <algorithm>

#include "../utils/projection.h"

namespace {
    constexpr float kFarBrightness = 0.45f;   // яркость атома у дальней стенки (у ближней - 1.0)
    constexpr int   kLabelFont     = 14;

    Color shade(Color c, float brightness) {
        return {static_cast<unsigned char>(c.r * brightness),
                static_cast<unsigned char>(c.g * brightness),
                static_cast<unsigned char>(c.b * brightness), c.a};
    }
}

void Render::DrawWorld(const Simulation& sim, float zoom, bool showGrid) const {
    // толщина делится на zoom, чтобы линии оставались ~1 px при любом масштабе
    const float thin = 1.0f / zoom;
    const float w = sim.Width();
    const float h = sim.Height();

    // Мировые координаты камеры: (x, -z), пол поля (z = 0) лежит на y = 0, потолок на y = -h
    if (showGrid) {
        Color c = Fade(DARKGRAY, 0.6f);
        for (int i = 1; i < sim.CellsX(); i++) {
            float x = i * Simulation::kCellSize;
            DrawLineEx({x, -h}, {x, 0.0f}, thin, c);
        }
        for (int j = 1; j < sim.CellsZ(); j++) {
            float y = -j * Simulation::kCellSize;
            DrawLineEx({0.0f, y}, {w, y}, thin, c);
        }
    }

    DrawRectangleLinesEx({0.0f, -h, w, h}, 2.0f / zoom, GRAY);
}

void Render::DrawAtoms(const Simulation& sim, const Camera2D& cam) const {
    const int sw = GetScreenWidth(), sh = GetScreenHeight();
    const float depth = sim.Depth();
    const auto& atoms = sim.Atoms();

    items.clear();
    for (unsigned int i = 0; i < atoms.size(); i++) {
        const Atom& a = atoms[i];
        const Vector2 s = GetWorldToScreen2D(Projection::ToPlane(a.position), cam);
        const float rPx = atomTypes[a.type].radius * Projection::DepthScale(a.position.y, depth) * cam.zoom;
        if (s.x < -rPx || s.y < -rPx || s.x > sw + rPx || s.y > sh + rPx) continue;
        items.push_back({a.position.y, s, i});
    }

    // Painter's algorithm: сначала дальние, потом ближние поверх них
    std::sort(items.begin(), items.end(), [](const Item& l, const Item& r) { return l.y < r.y; });

    for (const Item& it : items) {
        const Atom& a = atoms[it.id];
        const AtomData& d = atomTypes[a.type];

        const float t = Projection::DepthT(it.y, depth);
        const float rPx = d.radius * Projection::DepthScale(it.y, depth) * cam.zoom;

        DrawCircleV(it.screen, rPx, shade(d.color, kFarBrightness + (1.0f - kFarBrightness) * t));

        // если атом на экране слишком мелкий, подпись только мешает
        if (rPx >= kLabelFont * 0.6f) {
            const int w = MeasureText(d.symbol, kLabelFont);
            DrawText(d.symbol, int(it.screen.x) - w / 2, int(it.screen.y) - kLabelFont / 2, kLabelFont, BLACK);
        }
    }
}

void Render::DrawPlacementGhost(const Simulation& sim, const Camera2D& cam,
                                Vector2 worldPlane, float y, unsigned int type) const {
    const AtomData& d = atomTypes[type];
    const Vector2 s = GetWorldToScreen2D(worldPlane, cam);
    const float rPx = std::max(d.radius * Projection::DepthScale(y, sim.Depth()) * cam.zoom, 3.0f);

    DrawCircleLinesV(s, rPx, Fade(d.color, 0.9f));
}

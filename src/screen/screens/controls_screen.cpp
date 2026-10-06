#include "controls_screen.h"

namespace {
    struct Binding { const char* keys; const char* action; };

    constexpr Binding kBindings[] = {
        {"Enter",               "show / hide this list"},
        {"Z",                   "show / hide energy graph"},
        {"Space",               "pause"},
        {"+ / -",               "simulation speed x2 / x0.5"},
        {"Right ALT",           "single step (when paused)"},
        {"R",                   "reset all"},
        {"1 ... 0",             "select atom type"},
        {"LMB",                 "place atom"},
        {"RMB",                 "remove atom"},
        {"Q (hold)",            "cool atom system"},
        {"E (hold)",            "heat atom system"},
        {"[  /  ]  (hold)",     "placement depth: farther / nearer"},
        {"MMB",                 "drag atom (keeps depth) / pan camera"},
        {"Wheel",               "zoom"},
        {"W A S D",             "move camera"},
        {"-> / <-  (hold)",     "expand / compress field along X"},
        {"Up / Down (hold)",    "expand / compress field along Z"},
        {"Shift / Ctrl (hold)", "expand / compress field along Y (depth)"},
        {"'<' / '>' (hold)",    "gravity +/- 0.5 per step"},
        {"G",                   "toggle grid"},
    };

    constexpr int   kFont      = 18;
    constexpr float kRowHeight = 24.0f;
    constexpr float kKeyColumn = 210.0f;
    constexpr float kWidth     = 600.0f;
}

Vector2 ControlsScreen::Size() const {
    return {kWidth, kRowHeight * float(sizeof(kBindings) / sizeof(kBindings[0]))};
}

void ControlsScreen::Draw(const ScreenContext&, Rectangle area) {
    float y = area.y;
    for (const Binding& b : kBindings) {
        DrawText(b.keys,   int(area.x),              int(y), kFont, SKYBLUE);
        DrawText(b.action, int(area.x + kKeyColumn), int(y), kFont, RAYWHITE);
        y += kRowHeight;
    }
}

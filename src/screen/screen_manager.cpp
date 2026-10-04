#include "screen_manager.h"

#include <algorithm>

namespace {
    constexpr int   kMargin    = 50;   // отступ от левого края (как у остального UI)
    constexpr int   kBarHeight = 30;   // место под строку с горячими клавишами
    constexpr int   kBarFont   = 18;
    constexpr float kPad       = 12.0f;
    constexpr float kHeader    = 30.0f;
    constexpr int   kTitleFont = 20;
}

void ScreenManager::HandleInput() {
    for (int i = 0; i < int(entries.size()); i++) {
        if (IsKeyPressed(entries[i].key)) {
            toggle(i);
            break;   // одна клавиша за кадр
        }
    }
}

void ScreenManager::Update(const ScreenContext& ctx, float dt) {
    for (auto& e : entries) e.screen->Update(ctx, dt);
}

void ScreenManager::Close() {
    if (active < 0) return;
    entries[active].screen->OnClose();
    active = -1;
}

void ScreenManager::open(int index) {
    if (index == active) return;
    Close();
    active = index;
    entries[active].screen->OnOpen();
}

void ScreenManager::toggle(int index) {
    if (index == active) Close();
    else open(index);
}

void ScreenManager::Draw(const ScreenContext& ctx) const {
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    drawHotkeyBar(kMargin, sh - kBarHeight);
    if (active < 0) return;

    Screen& screen = *entries[active].screen;

    // Панель прижата к левому нижнему углу, над строкой подсказок
    const Vector2 want = screen.Size();
    const float maxW = float(sw - kMargin * 2);
    const float maxH = float(sh - kBarHeight - 10);

    Rectangle panel;
    panel.width  = std::min(want.x + kPad * 2.0f,            maxW);
    panel.height = std::min(want.y + kPad * 2.0f + kHeader,  maxH);
    panel.x = float(kMargin);
    panel.y = float(sh - kBarHeight - 10) - panel.height;

    DrawRectangleRec(panel, Fade(BLACK, 0.80f));
    DrawRectangleLinesEx(panel, 1.0f, GRAY);

    DrawText(screen.Title(), int(panel.x + kPad), int(panel.y + kPad - 2), kTitleFont, RAYWHITE);
    DrawLineEx({panel.x + kPad, panel.y + kHeader + kPad - 6.0f},
               {panel.x + panel.width - kPad, panel.y + kHeader + kPad - 6.0f},
               1.0f, DARKGRAY);

    const Rectangle area = {
        panel.x + kPad,
        panel.y + kPad + kHeader,
        panel.width  - kPad * 2.0f,
        panel.height - kPad * 2.0f - kHeader
    };
    screen.Draw(ctx, area);
}

void ScreenManager::drawHotkeyBar(int x, int y) const {
    for (int i = 0; i < int(entries.size()); i++) {
        const Color c = (i == active) ? RAYWHITE : GRAY;
        const char* text = TextFormat("[%s] %s", entries[i].keyLabel, entries[i].screen->Title());
        DrawText(text, x, y, kBarFont, c);
        x += MeasureText(text, kBarFont) + 24;
    }
}

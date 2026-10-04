#pragma once

#include "../screen.h"

// Список управления
class ControlsScreen : public Screen {
public:
    const char* Title() const override { return "Controls"; }
    Vector2 Size() const override;
    void Draw(const ScreenContext& ctx, Rectangle area) override;
};

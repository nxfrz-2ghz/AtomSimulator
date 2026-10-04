#pragma once

#include <raylib.h>
#include "../simulation/simulation.h"

// Всё, что экран может читать из приложения.
// Если экрану понадобится что-то ещё (камера, выбранный атом...) - добавлять сюда.
struct ScreenContext {
    const Simulation& sim;
};

// Базовый класс "экрана" - панели в левом нижнем углу.
// Рамку, заголовок и фон рисует ScreenManager, экран отвечает только за содержимое.
class Screen {
public:
    virtual ~Screen() = default;

    // Заголовок панели
    virtual const char* Title() const = 0;

    // Желаемый размер области содержимого (без рамки и заголовка).
    // Если окно меньше - менеджер сам ужмёт область.
    virtual Vector2 Size() const = 0;

    // Вызывается КАЖДЫЙ кадр, даже когда экран закрыт.
    // Нужен тем, кто копит данные в фоне (например, история энергии для графика).
    virtual void Update(const ScreenContext& ctx, float dt) {}

    // Вызывается только пока экран открыт. area - куда рисовать содержимое.
    virtual void Draw(const ScreenContext& ctx, Rectangle area) = 0;

    // Экран только что открылся / закрылся
    virtual void OnOpen() {}
    virtual void OnClose() {}
};

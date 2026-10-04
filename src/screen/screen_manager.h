#pragma once

#include <memory>
#include <utility>
#include <vector>
#include "screen.h"

// Владеет экранами, привязывает их к клавишам и показывает не более одного за раз.
//
//   Нажатие клавиши закрытого экрана  -> открывает его (текущий открытый закрывается)
//   Повторное нажатие той же клавиши  -> закрывает
class ScreenManager {
public:
    // Регистрирует экран на клавишу. keyLabel - как подписать клавишу в подсказке внизу.
    // Возвращает ссылку на созданный экран.
    template <class T, class... Args>
    T& Add(KeyboardKey key, const char* keyLabel, Args&&... args) {
        auto screen = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *screen;
        entries.push_back({key, keyLabel, std::move(screen)});
        return ref;
    }

    void HandleInput();                                  // горячие клавиши
    void Update(const ScreenContext& ctx, float dt);     // обновляет ВСЕ экраны
    void Draw(const ScreenContext& ctx) const;           // рисует подсказку и открытый экран

    void Close();
    bool HasActive() const { return active >= 0; }

private:
    struct Entry {
        KeyboardKey key;
        const char* keyLabel;
        std::unique_ptr<Screen> screen;
    };

    void open(int index);
    void toggle(int index);
    void drawHotkeyBar(int x, int y) const;

    std::vector<Entry> entries;
    int active = -1;   // индекс, а не указатель: vector может переаллоцироваться
};

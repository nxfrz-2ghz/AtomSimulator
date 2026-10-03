#pragma once

#include "../simulation/simulation.h"
#include "../render/render.h"
#include "../utils/hold_repeater.h"

class Manager {
public:
    Manager();

    void Update(float dt);
    void Draw2D();
    void DrawUI();

    void Shutdown() {};
private:
    Simulation simulation;
    Render render;
    Camera2D camera{};

    int selectedType = 0;
    bool showGrid = true;
    bool draggingAtom = false;
    Vector2 cameraVelocity = {0.0f, 0.0f};

    // Visual avg energy
    double shownKE = 0.0, shownPE = 0.0;
    size_t shownCount = 0;

    void handleInput(float dt);
    void handleTimeInput();
    void handleFieldResize(float dt);
    void smoothMovement(float dt);
    void updateEnergyDisplay(float dt);

    HoldRepeater placeHoldRepeater;
    HoldRepeater removeHoldRepeater;

    static constexpr float kFieldStep = 1.5f;
    HoldRepeater growRepeater{HoldRepeater::Config{0.3f, 0.30f, 0.01f, 8.0f}};
    HoldRepeater shrinkRepeater{HoldRepeater::Config{0.3f, 0.30f, 0.01f, 8.0f}};
};

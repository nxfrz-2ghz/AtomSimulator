#pragma once

#include "../simulation/simulation.h"
#include "../render/render.h"

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
    Vector2 cameraVelocity = {0.0f, 0.0f};

    // Visual avg energy
    double shownKE = 0.0, shownPE = 0.0;
    size_t shownCount = 0;

    void handleInput();
    void handleTimeInput();
    void smoothMovement(float dt);
    void updateEnergyDisplay(float dt);
};

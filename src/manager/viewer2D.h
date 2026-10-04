#pragma once

#include "../render/render.h"
#include "../utils/hold_repeater.h"
#include "../utils/projection.h"

class Viewer2D {
public:
    Viewer2D(Simulation& s) : simulation(s) {
        camera.zoom = 3.0f;
        camera.target = {simulation.Width() / 2.0f, -simulation.Height() / 2.0f};
        camera.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    }

    int selectedType = 0;
    float placeDepth = 0.5f;

    void Draw();
    void Update(float dt);

    void Reset() { draggingAtom = false; }

private:
    Simulation& simulation;
    Render render;

    Camera2D camera{};
    Vector2 cameraVelocity = {0.0f, 0.0f};

    bool showGrid = true;
    bool draggingAtom = false;

    HoldRepeater placeHoldRepeater;
    HoldRepeater removeHoldRepeater;
    static constexpr float kPlaceDepthStep = 0.02f;
    static constexpr HoldRepeater::Config kDepthConfig{0.3f, 0.10f, 0.02f, 2.0f};
    HoldRepeater placeFartherRepeater{kDepthConfig};
    HoldRepeater placeNearerRepeater{kDepthConfig};

    void handleInput(float dt);
    void smoothMovement(float dt);

    float placeY() const { return placeDepth * simulation.Depth(); }
};

#pragma once

#include "../simulation/simulation.h"
#include "../render/render.h"
#include "../utils/hold_repeater.h"
#include "../screen/screen_manager.h"

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
    ScreenManager screens;
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
    void handleThermostat(float dt);
    void smoothMovement(float dt);
    void updateEnergyDisplay(float dt);

    HoldRepeater placeHoldRepeater;
    HoldRepeater removeHoldRepeater;
    enum class ThermoAction { None, Cooling, Heating };
    static constexpr float kThermostatRate = 1.0f;
    ThermoAction thermoAction = ThermoAction::None;

    static constexpr float kFieldStep = 1.0f;
    static constexpr HoldRepeater::Config kResizeConfig{0.3f, 0.30f, 0.005f, 8.0f};
    HoldRepeater growRepeaters[3]   = {HoldRepeater{kResizeConfig}, HoldRepeater{kResizeConfig}, HoldRepeater{kResizeConfig}};
    HoldRepeater shrinkRepeaters[3] = {HoldRepeater{kResizeConfig}, HoldRepeater{kResizeConfig}, HoldRepeater{kResizeConfig}};

    static constexpr float kPlaceDepthStep = 0.02f;
    static constexpr HoldRepeater::Config kDepthConfig{0.3f, 0.10f, 0.02f, 2.0f};
    float placeDepth = 0.5f;
    HoldRepeater placeFartherRepeater{kDepthConfig};
    HoldRepeater placeNearerRepeater{kDepthConfig};

    float placeY() const;
};

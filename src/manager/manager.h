#pragma once

#include "../simulation/simulation.h"
#include "../screen/screen_manager.h"

#include "viewer2D.h"
#include "viewer3D.h"

class Manager {
public:
    Manager();

    void Update(float dt);
    void Draw();
    void DrawUI();

    void Shutdown() {};
private:
    Simulation simulation;
    ScreenManager screens;
    Viewer2D viewer2D;
    Viewer3D viewer3D;

    bool viewer2Dor3D = false;

    // Visual avg energy
    double shownKE = 0.0, shownPE = 0.0;
    size_t shownCount = 0;

    void handleInput(float dt);
    void updateEnergyDisplay(float dt);

    enum class ThermoAction { None, Cooling, Heating };
    static constexpr float kThermostatRate = 1.0f;
    ThermoAction thermoAction = ThermoAction::None;

    static constexpr float kFieldStep = 1.0f;
    static constexpr float kGravityStep = 0.5f;
    static constexpr HoldRepeater::Config kGravityConfig{ 0.25f, 0.15f, 0.05f, 1.0f };
    HoldRepeater gravityIncreaseRepeater{kGravityConfig};
    HoldRepeater gravityDecreaseRepeater{kGravityConfig};

    static constexpr HoldRepeater::Config kResizeConfig{ 0.2f, 0.2f, 0.001f, 6.0f };
    HoldRepeater growRepeaters[3]   = {HoldRepeater{kResizeConfig}, HoldRepeater{kResizeConfig}, HoldRepeater{kResizeConfig}};
    HoldRepeater shrinkRepeaters[3] = {HoldRepeater{kResizeConfig}, HoldRepeater{kResizeConfig}, HoldRepeater{kResizeConfig}};
};

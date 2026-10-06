#include "manager.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "../utils/projection.h"
#include "../screen/screens/controls_screen.h"
#include "../screen/screens/energy_graph_screen.h"

Manager::Manager():
    viewer2D(simulation),
    viewer3D(simulation)
{
    screens.Add<ControlsScreen>(KEY_ENTER, "Enter");
    screens.Add<EnergyGraphScreen>(KEY_Z, "Z");
}

void Manager::Update(float dt) {
    if (!viewer2Dor3D) viewer2D.Update(dt);
    else viewer3D.Update(dt);

    handleInput(dt);

    screens.HandleInput();
    simulation.Update(dt);
    updateEnergyDisplay(dt);
    screens.Update(ScreenContext{simulation}, dt);
}

void Manager::Draw() {
    if (!viewer2Dor3D) viewer2D.Draw();
    else viewer3D.Draw();
}

void Manager::DrawUI() {
    const AtomData& t = atomTypes[viewer2D.selectedType];
    const size_t n = simulation.Atoms().size();

    int posY = 50;
    const int stepY = 25;

    if (!viewer2Dor3D) {
        DrawText(TextFormat("Atom: %s", t.name), 50, posY, 20, RAYWHITE);
        posY += stepY;
    }

    // Atoms
    DrawText(TextFormat("Atoms: %d", static_cast<int>(n)), 50, posY, 20, RAYWHITE);
    posY += stepY;

    DrawText(TextFormat("Energy per atom: %.4f   (kin %.4f, pot %.4f)",
                        shownKE + shownPE, shownKE, shownPE),
             50, posY, 20, RAYWHITE);
    posY += stepY;

    // Energy/Temperature
    const char* totalText = TextFormat("Total Energy: %.3f", simulation.TotalEnergy());
    DrawText(totalText, 50, posY, 20, RAYWHITE);

    if (thermoAction != ThermoAction::None) {
        const bool cooling = thermoAction == ThermoAction::Cooling;
        DrawText(cooling ? "<< Cooling (Q)" : "Heating (E) >>",
                 50 + MeasureText(totalText, 20) + 20, posY, 20,
                 cooling ? SKYBLUE : ORANGE);
    }
    posY += stepY;

    // Time
    if (simulation.IsPaused()) {
        DrawText("PAUSED", 50, posY, 20, ORANGE);
    } else {
        DrawText(TextFormat("Speed: x%g%s", simulation.TimeScale(),
                            simulation.SpeedLimited() ? "  (CPU limit)" : ""),
                 50, posY, 20, simulation.SpeedLimited() ? RED : RAYWHITE);
    }
    posY += stepY;

    DrawText(TextFormat("Sim time: %.1f", simulation.SimTime()), 50, posY, 20, RAYWHITE);
    posY += stepY;

    // Field
    std::string moving;
    constexpr const char* kAxisNames[3] = {"X", "Y", "Z"};
    for (int a = 0; a < 3; a++) {
        const float v = simulation.WallSpeed(static_cast<Field::Axis>(a));
        if (v < -0.5f)     moving += TextFormat(" %s-", kAxisNames[a]);
        else if (v > 0.5f) moving += TextFormat(" %s+", kAxisNames[a]);
    }
    if (!moving.empty()) moving = " | Moving: " + moving;
    DrawText(
        TextFormat(
            "Field: X %.0f Y %.0f Z %.0f%s",
            simulation.Width(), simulation.Depth(), simulation.Height(), moving.c_str()
        ),
        50, posY, 20, RAYWHITE);
    posY += stepY;

    // Gravity
    DrawText(TextFormat("Gravity (Z-AXIS): %.1f", simulation.GetGravity()), 50, posY, 20, RAYWHITE);
    posY += stepY;

    if (!viewer2Dor3D) {
        DrawText(TextFormat("Place depth: %.0f%%", viewer2D.placeDepth * 100.0f), 50, posY, 20, RAYWHITE);
    }

    screens.Draw(ScreenContext{simulation});
}

void Manager::handleInput(float dt) {

    // Gravity
    const bool increaseGravity = IsKeyDown(KEY_PERIOD);
    const bool decreaseGravity = IsKeyDown(KEY_COMMA);

    if (gravityIncreaseRepeater.Update(dt, increaseGravity))
        simulation.SetGravity(simulation.GetGravity() + kGravityStep);
    if (gravityDecreaseRepeater.Update(dt, decreaseGravity))
        simulation.SetGravity(simulation.GetGravity() - kGravityStep);

    // Temperature
    const bool cool = IsKeyDown(KEY_Q);
    const bool heat = IsKeyDown(KEY_E);

    if (cool == heat || simulation.Atoms().empty()) {
        thermoAction = ThermoAction::None;
    }
    else {
        thermoAction = cool ? ThermoAction::Cooling : ThermoAction::Heating;
        const float sign = cool ? -1.0f : 1.0f;
        simulation.ScaleKineticEnergy(std::exp(sign * kThermostatRate * dt));
    }



    // Time
    if (IsKeyPressed(KEY_SPACE)) simulation.TogglePause();

    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD))
        simulation.SetTimeScale(simulation.TimeScale() * 2.0f);
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT))
        simulation.SetTimeScale(simulation.TimeScale() * 0.5f);
    if (IsKeyPressed(KEY_R)) {
        simulation.Reset();
        viewer2D.Reset();
    }

    if (simulation.IsPaused() &&
        (IsKeyPressed(KEY_RIGHT_ALT) || IsKeyPressedRepeat(KEY_RIGHT_ALT)))
        simulation.StepOnce();

    // FIELD RESIZE
    // X (ширина):  стрелки вправо / влево
    // Y (глубина): Shift / Ctrl
    // Z (высота):  стрелки вверх / вниз
    const bool grow[3] = {
        IsKeyDown(KEY_RIGHT),
        IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT),
        IsKeyDown(KEY_UP),
    };
    const bool shrink[3] = {
        IsKeyDown(KEY_LEFT),
        IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL),
        IsKeyDown(KEY_DOWN),
    };

    for (int a = 0; a < 3; a++) {
        const Field::Axis axis = static_cast<Field::Axis>(a);
        if (growRepeaters[a].Update(dt, grow[a]))     simulation.NudgeField(axis, +kFieldStep);
        if (shrinkRepeaters[a].Update(dt, shrink[a])) simulation.NudgeField(axis, -kFieldStep);
    }
}

void Manager::updateEnergyDisplay(float dt) {
    const size_t n = simulation.Atoms().size();
    if (n == 0) { shownKE = shownPE = 0.0; shownCount = 0; return; }

    const double ke = simulation.KineticEnergy()   / double(n);
    const double pe = simulation.PotentialEnergy() / double(n);

    if (n != shownCount) {
        shownKE = ke; shownPE = pe; shownCount = n;
    } else {
        const double a = 1.0 - std::exp(-double(dt) / 0.5);
        shownKE += (ke - shownKE) * a;
        shownPE += (pe - shownPE) * a;
    }
}

#include "manager.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "../utils/projection.h"
#include "../screen/screens/controls_screen.h"
#include "../screen/screens/energy_graph_screen.h"

Manager::Manager() {
    camera.zoom = 3.0f;
    // мировые координаты камеры: (x, -z), см. utils/projection.h
    camera.target = {simulation.Width() / 2.0f, -simulation.Height() / 2.0f};
    camera.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

    // Новый экран: класс от Screen + одна строка здесь
    screens.Add<ControlsScreen>(KEY_ENTER, "Enter");
    screens.Add<EnergyGraphScreen>(KEY_Z, "Z");
}

void Manager::Update(float dt) {
    handleInput(dt);
    screens.HandleInput();
    smoothMovement(dt);
    simulation.Update(dt);
    updateEnergyDisplay(dt);
    screens.Update(ScreenContext{simulation}, dt);
}

void Manager::Draw2D() {
    BeginMode2D(camera);
        render.DrawWorld(simulation, camera.zoom, showGrid);
    EndMode2D();

    render.DrawAtoms(simulation, camera);

    if (!draggingAtom) {
        const Vector2 world = GetScreenToWorld2D(GetMousePosition(), camera);
        render.DrawPlacementGhost(simulation, camera, world, placeY(), selectedType);
    }
}

void Manager::DrawUI() {
    const AtomData& t = atomTypes[selectedType];
    const size_t n = simulation.Atoms().size();

    DrawText(TextFormat("Atom: %s", t.name), 50, 50, 20, RAYWHITE);
    DrawText(TextFormat("Atoms: %d", static_cast<int>(n)), 50, 75, 20, RAYWHITE);

    DrawText(TextFormat("Energy per atom: %.4f   (kin %.4f, pot %.4f)",
                        shownKE + shownPE, shownKE, shownPE),
             50, 100, 20, RAYWHITE);
    const char* totalText = TextFormat("Total Energy: %.3f", simulation.TotalEnergy());
    DrawText(totalText, 50, 125, 20, RAYWHITE);

    // Текущее действие термостата - справа от энергии системы
    if (thermoAction != ThermoAction::None) {
        const bool cooling = thermoAction == ThermoAction::Cooling;
        DrawText(cooling ? "<< Cooling (Q)" : "Heating (E) >>",
                 50 + MeasureText(totalText, 20) + 20, 125, 20,
                 cooling ? SKYBLUE : ORANGE);
    }

    if (simulation.IsPaused()) {
        DrawText("PAUSED", 50, 150, 20, ORANGE);
    } else {
        DrawText(TextFormat("Speed: x%g%s", simulation.TimeScale(),
                            simulation.SpeedLimited() ? "  (CPU limit)" : ""),
                 50, 150, 20, simulation.SpeedLimited() ? RED : RAYWHITE);
    }
    DrawText(TextFormat("Sim time: %.1f", simulation.SimTime()), 50, 175, 20, RAYWHITE);

    DrawText(TextFormat("Field: X %.0f  Y %.0f  Z %.0f", simulation.Width(), simulation.Depth(), simulation.Height()),
             50, 200, 20, RAYWHITE);

    std::string moving;
    constexpr const char* kAxisNames[3] = {"X", "Y", "Z"};
    for (int a = 0; a < 3; a++) {
        const float v = simulation.WallSpeed(static_cast<Field::Axis>(a));
        if (v < -0.5f)     moving += TextFormat(" %s-", kAxisNames[a]);
        else if (v > 0.5f) moving += TextFormat(" %s+", kAxisNames[a]);
    }
    if (!moving.empty()) DrawText(("Walls:" + moving).c_str(), 50, 225, 20, GRAY);

    DrawText(TextFormat("Place depth: %.0f%%  ([ far, ] near)", placeDepth * 100.0f), 50, 250, 20, RAYWHITE);

    screens.Draw(ScreenContext{simulation});
}

void Manager::handleInput(float dt) {
    camera.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

    // Choose Atom
    for (int i = 0; i < int(atomTypes.size()); i++) {
        if (IsKeyPressed(KEY_ONE + i)) selectedType = i;
    }
    if (IsKeyPressed(KEY_ZERO)) selectedType = 9;

    if (IsKeyPressed(KEY_G)) showGrid = !showGrid;

    handleTimeInput();
    handleFieldResize(dt);

    // LMB: Create Atom
    if (placeHoldRepeater.Update(dt, IsMouseButtonDown(MOUSE_BUTTON_LEFT))) {
        Vector2 world = GetScreenToWorld2D(GetMousePosition(), camera);
        simulation.AddAtom(Projection::FromPlane(world, placeY()), selectedType);
    }

    // RMB: Remove Atom
    if (removeHoldRepeater.Update(dt, IsMouseButtonDown(MOUSE_BUTTON_RIGHT))) {
        Vector2 world = GetScreenToWorld2D(GetMousePosition(), camera);
        simulation.RemoveAtom(world);
    }

    // MMB: Drag atom and Throw
    if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
        Vector2 world = GetScreenToWorld2D(GetMousePosition(), camera);
        draggingAtom = simulation.BeginGrab(world, 10.0f / camera.zoom);
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        if (draggingAtom) {
            Vector2 world = GetScreenToWorld2D(GetMousePosition(), camera);
            simulation.DragTo(world, dt);
        } else {
            Vector2 delta = GetMouseDelta();
            camera.target.x -= delta.x / camera.zoom;
            camera.target.y -= delta.y / camera.zoom;
        }
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE)) {
        if (draggingAtom) simulation.EndGrab();
        draggingAtom = false;
    }

    // WHEEL: Zoom Camera
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        Vector2 before = GetScreenToWorld2D(GetMousePosition(), camera);
        camera.zoom *= (wheel > 0) ? 1.2f : 1.0f / 1.2f;
        if (camera.zoom < 0.1f) camera.zoom = 0.1f;
        if (camera.zoom > 100.0f) camera.zoom = 100.0f;
        Vector2 after = GetScreenToWorld2D(GetMousePosition(), camera);
        camera.target.x += before.x - after.x;
        camera.target.y += before.y - after.y;
    }


    handleThermostat(dt);

    if (placeFartherRepeater.Update(dt, IsKeyDown(KEY_LEFT_BRACKET)))
        placeDepth = std::max(placeDepth - kPlaceDepthStep, 0.02f);
    if (placeNearerRepeater.Update(dt, IsKeyDown(KEY_RIGHT_BRACKET)))
        placeDepth = std::min(placeDepth + kPlaceDepthStep, 0.98f);
}

void Manager::handleThermostat(float dt) {
    const bool cool = IsKeyDown(KEY_Q);
    const bool heat = IsKeyDown(KEY_E);

    // Обе клавиши сразу - действия гасят друг друга
    if (cool == heat || simulation.Atoms().empty()) {
        thermoAction = ThermoAction::None;
        return;
    }

    // Непрерывно, по реальному времени кадра: KE *= exp(+-rate * dt)
    thermoAction = cool ? ThermoAction::Cooling : ThermoAction::Heating;
    const float sign = cool ? -1.0f : 1.0f;
    simulation.ScaleKineticEnergy(std::exp(sign * kThermostatRate * dt));
}

void Manager::smoothMovement(float dt) {
    Vector2 direction = {0.0f, 0.0f};
    if (IsKeyDown(KEY_W)) direction.y -= 1.0f;
    if (IsKeyDown(KEY_S)) direction.y += 1.0f;
    if (IsKeyDown(KEY_A)) direction.x -= 1.0f;
    if (IsKeyDown(KEY_D)) direction.x += 1.0f;

    const float speed = 500.0f;
    const float acceleration = 8.0f;
    const float friction = 7.0f;

    Vector2 targetVelocity = {
        (direction.x * speed) / camera.zoom,
        (direction.y * speed) / camera.zoom
    };

    float blendFactor = (direction.x != 0.0f || direction.y != 0.0f) ? acceleration : friction;
    cameraVelocity.x += (targetVelocity.x - cameraVelocity.x) * blendFactor * dt;
    cameraVelocity.y += (targetVelocity.y - cameraVelocity.y) * blendFactor * dt;

    camera.target.x += cameraVelocity.x * dt;
    camera.target.y += cameraVelocity.y * dt;
}

void Manager::handleTimeInput() {
    if (IsKeyPressed(KEY_SPACE)) simulation.TogglePause();

    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD))
        simulation.SetTimeScale(simulation.TimeScale() * 2.0f);
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT))
        simulation.SetTimeScale(simulation.TimeScale() * 0.5f);
    if (IsKeyPressed(KEY_R)) {
        simulation.Reset();
        draggingAtom = false;
    }

    if (simulation.IsPaused() &&
        (IsKeyPressed(KEY_PERIOD) || IsKeyPressedRepeat(KEY_PERIOD)))
        simulation.StepOnce();
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

void Manager::handleFieldResize(float dt) {
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

float Manager::placeY() const {
    return placeDepth * simulation.Depth();
}

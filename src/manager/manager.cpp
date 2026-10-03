#include "manager.h"

#include <cmath>

Manager::Manager() {
    camera.zoom = 3.0f;
    camera.target = {simulation.Width() / 2.0f, simulation.Height() / 2.0f};
    camera.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
}

void Manager::Update(float dt) {
    handleInput(dt);
    smoothMovement(dt);
    simulation.Update(dt);
    updateEnergyDisplay(dt);
}

void Manager::Draw2D() {
    BeginMode2D(camera);
        render.DrawWorld(simulation, camera.zoom, showGrid);
        render.DrawAtoms(simulation);
    EndMode2D();
    render.DrawLabels(simulation, camera);
}

void Manager::DrawUI() {
    const AtomData& t = atomTypes[selectedType];
    const size_t n = simulation.Atoms().size();

    DrawText(TextFormat("Atom: %s", t.name), 50, 50, 20, RAYWHITE);
    DrawText(TextFormat("Atoms: %d", static_cast<int>(n)), 50, 75, 20, RAYWHITE);

    DrawText(TextFormat("Energy per atom: %.4f   (kin %.4f, pot %.4f)",
                        shownKE + shownPE, shownKE, shownPE),
             50, 100, 20, RAYWHITE);
    DrawText(TextFormat("Total Energy: %.3f", simulation.TotalEnergy()), 50, 125, 20, RAYWHITE);

    if (simulation.IsPaused()) {
        DrawText("PAUSED", 50, 150, 20, ORANGE);
    } else {
        DrawText(TextFormat("Speed: x%g%s", simulation.TimeScale(),
                            simulation.SpeedLimited() ? "  (CPU limit)" : ""),
                 50, 150, 20, simulation.SpeedLimited() ? RED : RAYWHITE);
    }
    DrawText(TextFormat("Sim time: %.1f", simulation.SimTime()), 50, 175, 20, RAYWHITE);

    const float ws = simulation.WallSpeed();
    DrawText(TextFormat("Field: %.0f x %.0f%s", simulation.Width(), simulation.Height(),
                        ws < -0.5f ? "   compressing" : (ws > 0.5f ? "   expanding" : "")),
             50, 200, 20, RAYWHITE);

    DrawText("Space: pause   +/- (or arrows): speed   R: reset all   . : step (when paused)",
             50, GetScreenHeight() - 55, 18, GRAY);
    DrawText("Shift / Ctrl (hold): expand / compress field   Q / E (hold): place / remove at cursor",
             50, GetScreenHeight() - 30, 18, GRAY);
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
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || placeHoldRepeater.Update(dt, IsKeyDown(KEY_Q))) {
        Vector2 world = GetScreenToWorld2D(GetMousePosition(), camera);
        simulation.AddAtom(world, selectedType);
    }

    // RMB: Remove Atom
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || removeHoldRepeater.Update(dt, IsKeyDown(KEY_E))) {
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
        if (camera.zoom > 70.0f) camera.zoom = 50.0f;
        Vector2 after = GetScreenToWorld2D(GetMousePosition(), camera);
        camera.target.x += before.x - after.x;
        camera.target.y += before.y - after.y;
    }
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

    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_RIGHT))
        simulation.SetTimeScale(simulation.TimeScale() * 2.0f);
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_LEFT))
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
    const bool grow   = IsKeyDown(KEY_LEFT_SHIFT)   || IsKeyDown(KEY_RIGHT_SHIFT);
    const bool shrink = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

    if (growRepeater.Update(dt, grow))     simulation.NudgeField(+kFieldStep);
    if (shrinkRepeater.Update(dt, shrink)) simulation.NudgeField(-kFieldStep);
}

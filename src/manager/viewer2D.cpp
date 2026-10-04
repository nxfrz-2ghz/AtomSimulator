#include "viewer2D.h"

void Viewer2D::Draw() {
    BeginMode2D(camera);
    render.DrawWorld(simulation, camera.zoom, showGrid);
    EndMode2D();

    render.DrawAtoms(simulation, camera);

    if (!draggingAtom) {
        const Vector2 world = GetScreenToWorld2D(GetMousePosition(), camera);
        render.DrawPlacementGhost(simulation, camera, world, placeY(), selectedType);
    }
}

void Viewer2D::Update(float dt) {
    handleInput(dt);
    smoothMovement(dt);
}

void Viewer2D::handleInput(float dt) {
    camera.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

    // Choose Atom
    for (int i = 0; i < int(atomTypes.size()); i++) {
        if (IsKeyPressed(KEY_ONE + i)) selectedType = i;
    }
    if (IsKeyPressed(KEY_ZERO)) selectedType = 9;

    if (IsKeyPressed(KEY_G)) showGrid = !showGrid;

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

    if (placeFartherRepeater.Update(dt, IsKeyDown(KEY_LEFT_BRACKET)))
        placeDepth = std::max(placeDepth - kPlaceDepthStep, 0.02f);
    if (placeNearerRepeater.Update(dt, IsKeyDown(KEY_RIGHT_BRACKET)))
        placeDepth = std::min(placeDepth + kPlaceDepthStep, 0.98f);
}

void Viewer2D::smoothMovement(float dt) {
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

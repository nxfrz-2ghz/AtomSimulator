#include <raylib.h>
#include "src/manager/manager.h"

int main() {

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "AtomSimulation");

    Manager manager;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        manager.Update(dt);

        BeginDrawing();
            ClearBackground(BLACK);
            manager.Draw2D();
            manager.DrawUI();
            DrawFPS(50, 20);
        EndDrawing();
    }

    manager.Shutdown();
    CloseWindow();
    return 0;
}
#pragma once

#include "../simulation/simulation.h" // временно

class Viewer3D {
public:
    Viewer3D(Simulation& s) : simulation(s) {}

    void Draw();
    void Update(float dt);

private:
    Simulation& simulation;
};

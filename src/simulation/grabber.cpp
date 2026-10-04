#include "grabber.h"

#include <algorithm>
#include <cmath>
#include <raymath.h>

namespace {
    constexpr float kMaxThrowSpeed = 30.0f;
    constexpr float kGrabSmoothing = 0.05f;
}

void Grabber::Begin(std::vector<Atom>& atoms, int id, Vector3 cursor) {
    grabbedId = id;
    grabOffset = Vector3Subtract(atoms[id].position, cursor);  // без "прыжка" к курсору
    grabVelocity = {0.0f, 0.0f, 0.0f};
    atoms[id].velocity = grabVelocity;
}

bool Grabber::DragTo(std::vector<Atom>& atoms, const Field& field,
                     Vector3 cursor, float realDt, float timeScale) {
    if (grabbedId < 0 || realDt <= 0.0f) return false;
    Atom& a = atoms[grabbedId];

    const Vector3 target = field.ClampInside(Vector3Add(cursor, grabOffset));

    const float simDt = realDt * timeScale;
    const Vector3 inst = {(target.x - a.position.x) / simDt,
                          (target.y - a.position.y) / simDt,
                          (target.z - a.position.z) / simDt};

    const float k = 1.0f - std::exp(-realDt / kGrabSmoothing);
    grabVelocity.x += (inst.x - grabVelocity.x) * k;
    grabVelocity.y += (inst.y - grabVelocity.y) * k;
    grabVelocity.z += (inst.z - grabVelocity.z) * k;

    const float speed = Vector3Length(grabVelocity);
    if (speed > kMaxThrowSpeed) grabVelocity = Vector3Scale(grabVelocity, kMaxThrowSpeed / speed);

    a.position = target;
    a.velocity = grabVelocity;
    return true;
}

void Grabber::End() {
    grabbedId = -1;
    grabVelocity = {0.0f, 0.0f, 0.0f};
}

void Grabber::OnAtomRemoved(unsigned int id, unsigned int last) {
    if (grabbedId == int(id)) End();
    if (grabbedId == int(last)) grabbedId = int(id);   // последний атом переезжает на место id
}

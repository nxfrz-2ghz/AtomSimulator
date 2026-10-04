#pragma once

#include <vector>
#include <raylib.h>
#include "../atom/atom.h"
#include "field.h"

class Grabber {
public:
    bool IsActive() const { return grabbedId >= 0; }
    int  Id()       const { return grabbedId; }   // -1, если ничего не схвачено

    void Begin(std::vector<Atom>& atoms, int id, Vector3 cursor);
    bool DragTo(std::vector<Atom>& atoms, const Field& field,
                Vector3 cursor, float realDt, float timeScale);
    void End();
    void Reset() { End(); }

    void OnAtomRemoved(unsigned int id, unsigned int last);

private:
    int     grabbedId    = -1;
    Vector3 grabOffset   = {0.0f, 0.0f, 0.0f};  // atom.pos - курсор в момент захвата
    Vector3 grabVelocity = {0.0f, 0.0f, 0.0f};  // сглаженная скорость (ед. мира / сим. сек)
};

#pragma once

#include <raylib.h>
#include <array>
#include <cstddef>

struct Atom {
    Vector3 position;
    Vector3 velocity;
    unsigned int type;

    Vector3 force; // для накопления сил на каждом шаге
};

inline float DistanceSqr(const Vector3& a, const Vector3& b) {
    const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

struct AtomData {
    float mass;
    float sigma;
    float epsilon;

    // Visual
    const char* name;
    const char* symbol;
    float radius;
    Color color;
};

inline constexpr std::array<AtomData, 10> atomTypes = {{
    { 1.008f,  2.500f, 0.125f, "Hydrogen", "H",  0.37f, { 255, 255, 255, 255 } }, // Белый
    { 4.003f,  2.556f, 0.084f, "Helium",   "He", 0.32f, { 0, 255, 255, 255 } },   // Циан
    { 12.011f, 3.500f, 0.276f, "Carbon",   "C",  0.77f, { 32, 32, 32, 255 } },    // Темно-серый
    { 14.007f, 3.250f, 0.711f, "Nitrogen", "N",  0.75f, { 0, 0, 255, 255 } },     // Синий
    { 15.999f, 2.960f, 0.878f, "Oxygen",   "O",  0.73f, { 255, 0, 0, 255 } },     // Красный
    { 18.998f, 2.940f, 0.255f, "Fluorine", "F",  0.71f, { 0, 255, 0, 255 } },     // Зеленый
    { 28.085f, 3.820f, 1.682f, "Silicon",  "Si", 1.11f, { 210, 180, 140, 255 } }, // Бежевый
    { 32.065f, 3.550f, 1.046f, "Sulfur",   "S",  1.02f, { 255, 255, 0, 255 } },   // Желтый
    { 35.453f, 3.470f, 1.255f, "Chlorine", "Cl", 0.99f, { 34, 139, 34, 255 } },   // Лесной зеленый
    { 196.967f, 2.934f, 2.213f, "Gold",    "Au", 1.44f, { 255, 215, 0, 255 } }    // Золотой
}};

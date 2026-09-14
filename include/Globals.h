#pragma once

class Globals
{
public:
    inline static float pixelsPerMeter = 20.0f;
    inline static float gravity = 981.0f;
    inline static bool isGravity = true;
    inline static bool GForce = true;
    inline static float coefficientOfRestitution = 0.0f;

    inline static float particleSize = 10.0f;
    inline static float particlesCount = 100.0f;
    inline static float particlesSpacing = 25.0f;
    inline static constexpr int PARTICLE_RESOLUTION = 32;
    inline static float particl_mass = 1.0f;
    inline static bool particles = true;
    inline static bool random = false;

    inline static bool circles = true;
    inline static float smoothingRadius = 20.0f;

    inline static float gridSize = 100.0f;
    inline static bool grid = false;

    inline static bool graph = false;

    inline static bool screenCollision = true;

    inline static double deltaTime = 0.0;
    inline static double PI = 3.14159265358979323846;

    inline static float sw;
    inline static float sh;
};
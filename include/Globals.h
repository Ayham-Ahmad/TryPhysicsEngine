#pragma once

class Globals
{
public:
    // --- Real-world (SI) units; pixelsPerMeter converts to screen pixels ---
    inline static float pixelsPerMeter = 300.0f;
    inline static float gravity = 9.8f;            // m/s^2 (Earth)
    inline static bool isGravity = true;
    inline static float coefficientOfRestitution = 0.05f;

    inline static float particleSize = 0.01f;      // m (radius)
    inline static float particlesCount = 200.0f;
    inline static float particlesSpacing = 0.02f;  // m
    inline static constexpr int PARTICLE_RESOLUTION = 32;
    inline static float particleMass = 0.4f;       // kg
    inline static bool particles = true;
    inline static bool random = true;

    inline static bool circles = true;
    inline static float smoothingRadius = 0.08f;   // m (4x the spacing -> ~50 neighbours)
    inline static bool gradientArrows = false;
    inline static float targetDensity = 1000.0f;   // kg/m^3 (water)
    inline static float pressureMultiplier = 0.5f;

    // Speed colouring
    inline static bool speedColoring = true;
    inline static float velocityMax = 20.0f;       // m/s, normalises speed into the ramp

    inline static float gridSize = 100.0f;
    inline static bool grid = false;

    inline static bool graph = false;

    inline static bool screenCollision = true;

    inline static double deltaTime = 0.0;
    inline static float PI = 3.14159265f;

    inline static float sw;
    inline static float sh;

    inline static float worldW = 0.0f;
    inline static float worldH = 0.0f;
};
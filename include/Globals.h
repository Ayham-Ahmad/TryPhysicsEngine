#pragma once

class Globals
{
public:
    // --- Real-world (SI) units; pixelsPerMeter converts to screen pixels ---
    inline static float pixelsPerMeter = 300.0f;
    inline static float gravity = 9.8f; // m/s^2 (Earth)
    inline static bool isGravity = true;
    inline static float coefficientOfRestitution = 1.0f;

    inline static float particleSize = 0.004f; // m (radius)
    inline static float particlesCount = 9000.774f;
    // The lattice pitch used by Particle::init is particlesSpacing + 2 * particleSize
    inline static float particlesSpacing = 0.02f; // m (gap between particle surfaces)
    inline static constexpr int PARTICLE_RESOLUTION = 32;
    inline static float particleMass = 0.4f; // kg
    inline static bool particles = true;
    inline static bool random = false;

    inline static bool circles = true;
    inline static float smoothingRadius = 0.08f; // m (4x the spacing -> ~50 neighbours)
    inline static bool gradientArrows = false;
    inline static float pressureMultiplier = 0.5f;

    // Speed colouring
    inline static bool speedColoring = true;
    inline static float velocityMax = 20.0f; // m/s, normalises speed into the ramp

    inline static float gridSize = 100.0f;
    inline static bool grid = false;

    // --- Double density relaxation (Clavet et al. 2005) ---
    // Neighbours inside interatctionRadius contribute to two raw metrics
    // (q = distance / interatctionRadius):
    //     density     = sum((1 - q)^2)
    //     densityNear = sum((1 - q)^3)
    // and every pair is pushed apart by
    //     dt^2 * (pressure * (1 - q) + pressureNear * (1 - q)^2)
    // with pressure = k * (density - targetDensity), pressureNear = kNear * densityNear.

    // Interaction radius h in metres: how far a particle can "see".
    // Larger -> more neighbours (smoother, stiffer, slower); smaller -> cheaper,
    // grainier, and the fluid can slip through gaps. Rule of thumb: 2-3x the
    // particle pitch (particlesSpacing + 2 * particleSize = 0.028 m here).
    inline static float interatctionRadius = 0.078f;

    // Density the fluid relaxes towards: pressure = k * (density - targetDensity).
    // density is the raw, scale-free metric above, so its lattice value depends
    // only on interatctionRadius and the pitch (2.93 for the values here).
    // Above that value the fluid pulls itself together (cohesive/contracting),
    // below it the fluid pushes itself apart (expanding).
    inline static float targetDensity = 4.05f;

    // Near pressure stiffness: kNear * densityNear. It only acts on the closest
    // neighbours, so it stops particles collapsing into each other and provides
    // surface tension. Too high -> jitter/shatter, too low -> clumping.
    inline static float kNear = 30.0f;

    // Bulk pressure stiffness: k * (density - targetDensity), the spring constant
    // of the liquid. Small k -> soft, slow, gooey; large k -> stiff and snappy.
    // The correction is scaled by dt^2, so with dt = 1/120 s it must be in the
    // tens-to-hundreds: below ~1 the fluid sinks through itself, above ~500 it
    // explodes.
    inline static float k = 65.0f;

    inline static bool graph = false;

    inline static bool screenCollision = true;

    inline static double deltaTime = 0.0;
    inline static float PI = 3.14159265f;

    inline static float sw;
    inline static float sh;

    inline static float worldW = 0.0f;
    inline static float worldH = 0.0f;
};
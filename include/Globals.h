#pragma once

class Globals
{
public:
    // Change this value to choose how many simulations run.
    inline static constexpr int simCount = 8;
    static_assert(simCount > 0, "simCount must be greater than zero");
    inline static bool simBox = false;

    // --- Real-world (SI) units; pixelsPerMeter converts to screen pixels ---
    inline static float pixelsPerMeter = 300.0f;
    inline static float gravity = 9.8f; // m/s^2 (Earth)
    inline static bool isGravity = true;
    inline static float coefficientOfRestitution = 1.0f;

    inline static float particleSize = 0.004f; // m (radius)
    inline static float particlesCount = 9000.774f;
    // The lattice pitch used by Particle::init is particlesSpacing + 2 * particleSize
    inline static float particlesSpacing = 0.02f; // m (gap between particle surfaces)
    inline static int PARTICLE_RESOLUTION = 32;
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

    // Viscosity (Clavet et al. 2005). Particles that are moving towards each other
    // get a damping impulse
    //     impulse = dt * (1 - q) * (Sigma * u + Beta * u * u)
    // where u is the closing speed along the line between them. Both constants are
    // multiplied by dt (1/120 s here), so they have to be large to be feelable:
    // ~0 is water, a few hundred is syrup, ~300 is honey, thousands turn the
    // fluid into a near-solid.
    inline static float Sigma = 0.0f;
    inline static float Beta = 0.0f;

    // --- Spring plasticity (Clavet et al. 2005) ---
    // A spring is created between every pair of particles closer than
    // interatctionRadius. It behaves like a tiny elastic band: when the pair is
    // compressed it pushes them apart, when it is stretched it pulls them together.
    // The passes run right after the density pass, so springs add the elastic /
    // gooey part on top of the fluid part.

    // gamma: dead zone around the rest length, as a fraction of it (unitless).
    // The rest length is only adjusted when the pair's actual distance drifts more
    // than gamma * restLength away from it, so 0.13 means the inner +-13% is
    // ignored. Small -> springs react to tiny changes (softer, more jelly-like);
    // large -> springs ignore small drifts and feel stiffer.
    inline static float gamma = 0.13f;

    // plasticity: how fast a rest length follows the real distance, per second.
    // It is multiplied by dt, so at dt = 1/120 s a value of 100 closes ~83% of the
    // gap each step. Small -> springs keep their original length (elastic memory,
    // the fluid bounces back); large -> springs permanently deform to whatever
    // shape the fluid is squeezed into (plastic, keeps the new shape).
    inline static float plasticity = 100.0f;

    // springStiffness: how hard a spring corrects towards its rest length. The
    // displacement is scaled by dt^2 * springStiffness * (1 - L/h) * (L - distance),
    // so at dt = 1/120 s it must be in the thousands to be visible next to the
    // density pass: below ~1000 the springs are lost in the noise, ~20000 gives a
    // clearly elastic fluid, and above ~50000 the spring pass starts to explode.
    inline static float springStiffness = 20000.0f;

    inline static bool graph = false;

    inline static bool emitters = true;

    inline static bool screenCollision = true;

    inline static double deltaTime = 0.0;
    inline static float PI = 3.14159265f;

    inline static float sw;
    inline static float sh;

    inline static float worldW = 0.0f;
    inline static float worldH = 0.0f;
};
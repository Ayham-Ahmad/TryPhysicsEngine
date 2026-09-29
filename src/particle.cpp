#include "particle.h"

namespace
{
    // Metrics are recomputed at most ~10Hz (see Particle::getPhysicsStats)
    constexpr float STATS_RATE = 1.0f / 10.0f;

    // Colour ramp matching the video's SpeedColMap (blue -> cyan -> green -> yellow -> red),
    // sampled at t = saturate(speed / velocityMax)
    SDL_FColor speedRampColor(const float t)
    {
        struct Stop
        {
            float r, g, b;
        };

        static const Stop stops[] = {
            {0.05f, 0.10f, 0.85f}, // slow: blue
            {0.10f, 0.65f, 1.00f}, // cyan
            {0.15f, 0.85f, 0.45f}, // green
            {0.95f, 0.80f, 0.15f}, // yellow
            {0.95f, 0.25f, 0.15f}, // fast: red
        };

        constexpr int N = sizeof(stops) / sizeof(stops[0]);

        const float scaled = t * static_cast<float>(N - 1);
        const int segment = std::min(static_cast<int>(scaled), N - 2);
        const float f = scaled - static_cast<float>(segment);

        const Stop &a = stops[segment];
        const Stop &b = stops[segment + 1];

        return {
            a.r + (b.r - a.r) * f,
            a.g + (b.g - a.g) * f,
            a.b + (b.b - a.b) * f,
            1.0f};
    }
}

void Particle::init(SDL_Renderer *r)
{
    // rows and cols for grid of particles
    int particlesPerRow = (int)sqrt(Globals::particlesCount);
    int particlesPerCol = (Globals::particlesCount - 1) / particlesPerRow + 1;
    const float particlesSpacing = Globals::particlesSpacing + Globals::particleSize * 2;

    _particles.clear();

    // Create each particle
    for (int i = 0; i < Globals::particlesCount; i++)
    {
        ParticleObject p;

        if (Globals::random)
        {
            // Set a random position
            p.position.x = randomFloat(
                Globals::particleSize,
                Globals::worldW - Globals::particleSize);

            p.position.y = randomFloat(
                Globals::particleSize,
                Globals::worldH - Globals::particleSize);
        }
        else
        {
            // Calculate the row and column
            float x = Globals::worldW / 2.0f +
                      (i % particlesPerRow - particlesPerRow / 2.0f + 0.5f) * particlesSpacing;

            float y = Globals::worldH / 2.0f +
                      (i / particlesPerRow - particlesPerCol / 2.0f + 0.5f) * particlesSpacing;

            // Set the particle position
            p.position.x = x;
            p.position.y = y;
        }

        _particles.push_back(p);
    }

    // Force the metrics cache to recompute (count or layout changed)
    _statsTimer = STATS_RATE;
}

void Particle::update(const double simulationDeltaTime)
{
    const size_t count = _particles.size();
    const float deltaTime = static_cast<float>(simulationDeltaTime);

    constexpr float predictionDeltaTime = 1.0f / 120.0f;

    _densities.resize(count);
    _positions.resize(count);

    for (size_t i = 0; i < count; i++)
    {
        ParticleObject &p = _particles[i];

        if (Globals::isGravity)
        {
            p.velocity.y += Globals::gravity * deltaTime;
            p.acceleration.y = Globals::gravity;
        }

        _positions[i].x = p.position.x + p.velocity.x * predictionDeltaTime;
        _positions[i].y = p.position.y + p.velocity.y * predictionDeltaTime;
    }

    // Step 2: Rebuild the spatial lookup with the predicted positions
    _spatialHash.update(_positions, Globals::smoothingRadius);

    // Step 3: Calculate densities at the predicted positions
    for (size_t i = 0; i < count; i++)
        _densities[i] = _fluid.calculateDensity(_positions[i], _spatialHash);

    // Step 4: Calculate and apply pressure forces
    for (size_t i = 0; i < count; i++)
    {
        const float density = _densities[i];
        if (density <= 0.0f)
            continue;

        const SDL_FPoint pressureForce = _fluid.calculatePressureForce(_positions[i], density, _positions, _densities, _spatialHash);

        const SDL_FPoint pressureAcceleration = {
            pressureForce.x / density,
            pressureForce.y / density};

        ParticleObject &p = _particles[i];

        p.velocity.x += pressureAcceleration.x * deltaTime;
        p.velocity.y += pressureAcceleration.y * deltaTime;

        p.acceleration.x += pressureAcceleration.x;
        p.acceleration.y += pressureAcceleration.y;
    }

    // Step 5: Update positions and resolve collisions
    for (size_t i = 0; i < count; i++)
    {
        ParticleObject &p = _particles[i];

        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;

        Physics2D::checkCollisionBetweenObjAndScreen(p);
    }
}

void Particle::render(SDL_Renderer *r) const
{
    const size_t count = _particles.size();
    const bool hasDensities = _densities.size() == count;

    // Normalize the pressure colors: max |density - targetDensity| over all particles
    // (skipped entirely while the speed colour ramp is active)
    float maxAbs = 0.0f;
    if (hasDensities && !Globals::speedColoring)
    {
        for (size_t i = 0; i < count; i++)
            maxAbs = std::max(maxAbs, std::abs(_densities[i] - Globals::targetDensity));
    }

    const float invMaxAbs = maxAbs < 1e-12f ? 0.0f : 1.0f / maxAbs;

    const auto particleColor = [&](const size_t i) -> SDL_FColor
    {
        if (Globals::speedColoring)
        {
            const ParticleObject &p = _particles[i];
            const float speed = std::sqrt(p.velocity.x * p.velocity.x + p.velocity.y * p.velocity.y);
            const float speedT = std::min(1.0f, speed / Globals::velocityMax);
            return speedRampColor(speedT);
        }

        const float pressure = hasDensities ? _densities[i] - Globals::targetDensity : 0.0f;
        const float t = std::max(-1.0f, std::min(1.0f, pressure * invMaxAbs));

        SDL_FColor color;
        if (t <= 0.0f)
        {
            color.r = 1.0f + t;
            color.g = 1.0f + t;
            color.b = 1.0f;
        }
        else
        {
            color.r = 1.0f;
            color.g = 1.0f - t;
            color.b = 1.0f - t;
        }

        color.a = 1.0f;
        return color;
    };

    if (Globals::circles)
        Shape::glowField(r, _particles.begin(), _particles.end(), particleColor, _glow);

    // Render each particle
    if (Globals::particles)
    {
        const float radius = Globals::particleSize * Globals::pixelsPerMeter;
        for (size_t i = 0; i < count; i++)
        {
            const ParticleObject &p = _particles[i];

            const SDL_FPoint posPx = {
                p.position.x * Globals::pixelsPerMeter,
                p.position.y * Globals::pixelsPerMeter};

            Shape::shape(r, posPx, radius, Globals::PARTICLE_RESOLUTION, particleColor(i));
            // Shape::shape(r, posPx, radius, Globals::PARTICLE_RESOLUTION, Colors::CyanF());
        }
    }

    // Render the property gradient field as arrows on a screen-wide grid
    // if (Globals::gradientArrows)
    // {
    //     _fieldRenderer.update(_particles, static_cast<float>(Globals::deltaTime));
    //     _fieldRenderer.renderArrows(r);
    // }
}

const std::vector<ParticleObject> &Particle::getParticlesList() const
{
    return _particles;
}

PhysicsStats Particle::getPhysicsStats() const
{
    // 10 times per sec
    _statsTimer += static_cast<float>(Globals::deltaTime);
    if (_statsTimer < STATS_RATE && _particles.size() == _densities.size())
        return _cachedStats;
    _statsTimer = 0.0f;

    static constexpr int BINS = 10;
    static constexpr float MAX_SPEED = 30.0f;

    const size_t count = _particles.size();

    int mergedCounts[BINS] = {};
    float speedSum = 0.0f;
    float kineticEnergySum = 0.0f;
    float accelerationSum = 0.0f;
    float densitySum = 0.0f;

    // Fallback neighbour lookup, only needed when the cached densities are stale
    // (e.g. right after a reset and before the first simulation step)
    std::vector<SDL_FPoint> positions;
    SpatialHash hash;
    const bool hasDensities = _densities.size() == count;

    if (!hasDensities)
    {
        positions.resize(count);
        for (size_t i = 0; i < count; i++)
        {
            positions[i].x = _particles[i].position.x;
            positions[i].y = _particles[i].position.y;
        }

        hash.update(positions, Globals::smoothingRadius);
    }

    for (size_t i = 0; i < count; i++)
    {
        const ParticleObject &p = _particles[i];

        float speed = std::sqrt(p.velocity.x * p.velocity.x + p.velocity.y * p.velocity.y);
        float acceleration = std::sqrt(p.acceleration.x * p.acceleration.x + p.acceleration.y * p.acceleration.y);

        speedSum += speed;
        kineticEnergySum += 0.5f * Globals::particleMass * speed * speed;
        accelerationSum += acceleration;

        // Reuse the densities computed by the latest simulation step instead of
        // running a density sweep once per frame; fall back to a fresh
        // neighbour lookup when the cache is stale
        if (hasDensities)
            densitySum += _densities[i];
        else
            densitySum += _fluid.calculateDensity(positions[i], hash);

        int bin = static_cast<int>((speed / MAX_SPEED) * BINS);

        if (bin >= BINS)
            bin = BINS - 1;

        mergedCounts[bin]++;
    }

    // Calculate Shannon entropy
    float entropy = 0.0f;

    for (int bin = 0; bin < BINS; bin++)
    {
        if (mergedCounts[bin] > 0)
        {
            float probability = static_cast<float>(mergedCounts[bin]) / count;

            entropy -= probability * std::log(probability);
        }
    }

    const float divider = _particles.empty() ? 1.0f : static_cast<float>(count);

    _cachedStats = {
        entropy / std::logf(BINS),
        speedSum / divider,
        kineticEnergySum / divider,
        accelerationSum / divider,
        densitySum / divider,
    };

    return _cachedStats;
}
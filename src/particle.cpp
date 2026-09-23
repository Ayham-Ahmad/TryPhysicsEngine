#include "particle.h"

namespace
{
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
    // The sim runs in meters; the screen (pixels) is converted once here
    const float worldW = Globals::sw / Globals::pixelsPerMeter;
    const float worldH = Globals::sh / Globals::pixelsPerMeter;

    int particlesPerRow = (int)sqrt(Globals::particlesCount);
    int particlesPerCol = (Globals::particlesCount - 1) / particlesPerRow + 1;
    const float particlesSpacing = Globals::particlesSpacing + Globals::particleSize * 2;

    _particles.clear();

    // Create each particle
    for (int i = 0; i < Globals::particlesCount; i++)
    {
        ParticleObject p;

        p.position = {0, 0};

        if (Globals::random)
        {
            // Set a random position
            p.position.x = randomFloat(
                Globals::particleSize,
                worldW - Globals::particleSize);

            p.position.y = randomFloat(
                Globals::particleSize,
                worldH - Globals::particleSize);
        }
        else
        {
            // Calculate the row and column
            float x = worldW / 2.0f +
                      (i % particlesPerRow - particlesPerRow / 2.0f + 0.5f) * particlesSpacing;

            float y = worldH / 2.0f +
                      (i / particlesPerRow - particlesPerCol / 2.0f + 0.5f) * particlesSpacing;

            // Set the particle position
            p.position.x = x;
            p.position.y = y;
        }

        _particles.push_back(p);
    }
}

void Particle::update(const double simulationDeltaTime)
{
    const size_t count = _particles.size();
    const float deltaTime = static_cast<float>(simulationDeltaTime);

    // The prediction look-ahead is a fixed 1/120s substep in the video, independent
    // of deltaTime, so the neighbour search distance never changes with frame pacing
    constexpr float predictionDeltaTime = 1.0f / 120.0f;

    _densities.resize(count);
    _positions.resize(count);

    // Step 1: Apply gravity and predict the next positions. The predicted positions
    // (stored in _positions) are what the spatial lookup, densities and pressure
    // forces are all built on; the real positions move in step 5.
    parallelFor(count, [&](const size_t i)
                {
        ParticleObject &p = _particles[i];

        p.acceleration = {0.0f, 0.0f};

        if (Globals::isGravity)
        {
            p.velocity.y += Globals::gravity * deltaTime;
            p.acceleration.y = Globals::gravity;
        }

        _positions[i].x = p.position.x + p.velocity.x * predictionDeltaTime;
        _positions[i].y = p.position.y + p.velocity.y * predictionDeltaTime; });

    // Step 2: Rebuild the spatial lookup with the predicted positions
    _spatialHash.update(_positions, Globals::smoothingRadius);

    // Step 3: Calculate densities at the predicted positions
    parallelFor(count, [&](const size_t i)
                { _densities[i] = _collision.calculateDensity(_positions[i], _spatialHash); });

    // Step 4: Calculate and apply pressure forces
    parallelFor(count, [&](const size_t i)
                {
        const float density = _densities[i];
        if (density <= 0.0f)
            return;

        const SDL_FPoint pressureForce = _collision.calculatePressureForce(_positions[i], density, _positions, _densities, _spatialHash);

        const SDL_FPoint pressureAcceleration = {
            pressureForce.x / density,
            pressureForce.y / density};

        ParticleObject &p = _particles[i];

        p.velocity.x += pressureAcceleration.x * deltaTime;
        p.velocity.y += pressureAcceleration.y * deltaTime;

        p.acceleration.x += pressureAcceleration.x;
        p.acceleration.y += pressureAcceleration.y; });

    // Step 5: Update positions and resolve collisions
    parallelFor(count, [&](const size_t i)
                {
        ParticleObject &p = _particles[i];

        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;

        _collision.checkCollisionBwteenObjAndScreen(p); });
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

    // Speed mode: sample the colour ramp at speed / velocityMax (the video's shader
    // does the same with a SpeedColMap texture). Pressure mode: blue = below target
    // density, white = at target, red = above target.
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

    // Render each Circle
    if (Globals::circles)
    {
        SDL_Texture *texture = getCircleTexture(r);

        const float outer = (Globals::particleSize + Globals::smoothingRadius) * Globals::pixelsPerMeter;

        for (size_t i = 0; i < count; i++)
        {
            const SDL_FColor color = particleColor(i);

            SDL_SetTextureColorMod(
                texture,
                static_cast<Uint8>(color.r * 255.0f),
                static_cast<Uint8>(color.g * 255.0f),
                static_cast<Uint8>(color.b * 255.0f));

            const ParticleObject &p = _particles[i];

            SDL_FRect dst = {
                p.position.x * Globals::pixelsPerMeter - outer,
                p.position.y * Globals::pixelsPerMeter - outer,
                outer * 2,
                outer * 2};

            SDL_RenderTexture(r, texture, nullptr, &dst);
        }

        // Restore unmodulated rendering for any later texture draws
        SDL_SetTextureColorMod(texture, 255, 255, 255);
    }

    // Render each particle
    if (Globals::particles)
        for (size_t i = 0; i < count; i++)
        {
            const ParticleObject &p = _particles[i];

            const SDL_FPoint posPx = {
                p.position.x * Globals::pixelsPerMeter,
                p.position.y * Globals::pixelsPerMeter};

            Shape::shape(r, posPx, Globals::particleSize * Globals::pixelsPerMeter, Globals::PARTICLE_RESOLUTION, particleColor(i));
        }

    // Render the property gradient field as arrows on a screen-wide grid
    if (Globals::gradientArrows)
    {
        _arrowRefreshTimer += static_cast<float>(Globals::deltaTime);
        if (_arrowRefreshTimer >= 1.0f / 30.0f)
        {
            _arrowRefreshTimer = 0.0f;
            refreshGradientArrows();
        }

        for (const auto &a : _arrowCache)
            Shape::arrow(r, a.pos, a.tip);
    }

    // SDL_FRect rect1 = {_screenWidth / 2.0f - 80, _screenHeight / 2.0f - 50, 30, 30};
    // SDL_FRect rect2 = {_screenWidth / 2.0f + 20, _screenHeight / 2.0f - 80, 40, 40};
    // SDL_FRect rect3 = {_screenWidth / 2.0f + 10, _screenHeight / 2.0f + 40, 25, 25};
    // SDL_FColor color = {34 / 255.0f, 139 / 255.0f, 34 / 255.0f, 1.0f};

    // SDL_SetRenderDrawColorFloat(r, color.r, color.g, color.b, color.a);
    // SDL_RenderFillRect(r, &rect1);
    // SDL_RenderFillRect(r, &rect2);
    // SDL_RenderFillRect(r, &rect3);
}

// Get the circle texture, regenerating it only when the smoothing radius or the particle size changes
SDL_Texture *Particle::getCircleTexture(SDL_Renderer *r) const
{
    const float smoothing = Globals::smoothingRadius;
    const float particleSize = Globals::particleSize;

    if (circleTexture && circleTextureSmoothing == smoothing && circleTextureParticleSize == particleSize)
        return circleTexture;

    if (circleTexture)
        SDL_DestroyTexture(circleTexture);

    // The visual field is drawn in pixels, the sim is in meters
    float outer = (Globals::particleSize + Globals::smoothingRadius) * Globals::pixelsPerMeter;

    circleTexture = Shape::circleTexture(r, outer);
    circleTextureSmoothing = smoothing;
    circleTextureParticleSize = particleSize;

    return circleTexture;
}

const std::vector<ParticleObject> &Particle::getParticlesList() const
{
    return _particles;
}

void Particle::refreshGradientArrows() const
{
    _arrowCache.clear();

    // Sample grid in world (meter) units; ~0.2 m == 60 px at 300 px/m
    const float spacing = 0.2f;

    // Arrows start at the box center, so half the spacing keeps them inside their own grid box
    const float MAX_ARROW_LENGTH = spacing * 0.5f;

    const float ppu = Globals::pixelsPerMeter;
    const float worldW = Globals::sw / ppu;
    const float worldH = Globals::sh / ppu;

    struct Sample
    {
        SDL_FPoint pos;
        SDL_FPoint dir;
        float magnitude;
    };

    std::vector<Sample> samples;

    // Build a spatial lookup from the current particle positions (in meters) so the
    // per-sample density and force queries stay neighbour-only.
    std::vector<SDL_FPoint> positions(_particles.size());
    parallelFor(_particles.size(), [&](const size_t i)
                {
        positions[i] = {_particles[i].position.x, _particles[i].position.y}; });

    SpatialHash hash;
    hash.update(positions, Globals::smoothingRadius);

    // Densities ρ_i only depend on particle positions (frozen during this refresh), so
    // compute all N once in parallel instead of N times per gradient call.
    std::vector<float> densities(_particles.size());
    parallelFor(_particles.size(), [&](const size_t i)
                { densities[i] = _collision.calculateDensity(positions[i], hash); });

    for (float y = spacing * 0.5f; y < worldH; y += spacing)
    {
        for (float x = spacing * 0.5f; x < worldW; x += spacing)
        {
            const SDL_FPoint sample = {x, y};
            const float sampleDensity = _collision.calculateDensity(sample, hash);
            const SDL_FPoint gradient = _collision.calculatePressureForce(sample, sampleDensity, positions, densities, hash);

            const float magnitude = std::sqrt(gradient.x * gradient.x + gradient.y * gradient.y);
            if (magnitude < 1e-9f)
                continue;

            samples.push_back({sample,
                               {gradient.x / magnitude, gradient.y / magnitude},
                               magnitude});
        }
    }

    float maxMagnitude = 0.0f;
    for (const Sample &s : samples)
        maxMagnitude = std::max(maxMagnitude, s.magnitude);

    if (maxMagnitude < 1e-9f)
        return;

    for (const Sample &s : samples)
    {
        const float length = (s.magnitude / maxMagnitude) * MAX_ARROW_LENGTH;

        // Arrow geometry is sampled in meters, drawn in pixels
        if (length < 3.0f / ppu)
        {
            const SDL_FPoint posPx = {s.pos.x * ppu, s.pos.y * ppu};
            _arrowCache.emplace_back(GridArrow{posPx, posPx});
            continue;
        }

        const SDL_FPoint posPx = {s.pos.x * ppu, s.pos.y * ppu};
        const SDL_FPoint tipPx = {
            (s.pos.x + s.dir.x * length) * ppu,
            (s.pos.y + s.dir.y * length) * ppu};

        _arrowCache.emplace_back(GridArrow{posPx, tipPx});
    }
}

PhysicsStats Particle::getPhysicsStats() const
{
    static constexpr int BINS = 10;
    static constexpr float MAX_SPEED = 30.0f;

    const size_t count = _particles.size();

    // Number of per-thread accumulators
    unsigned threads = std::thread::hardware_concurrency();
    if (threads == 0)
        threads = 1;

    // Every thread adds into its OWN private accumulator (avoids data races)
    std::vector<int> counts(threads * BINS, 0);
    std::vector<float> totalSpeed(threads, 0.0f);
    std::vector<float> totalKineticEnergy(threads, 0.0f);
    std::vector<float> totalAcceleration(threads, 0.0f);
    std::vector<float> totalDensity(threads, 0.0f);

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

    parallelChunks(count, [&](const size_t begin, const size_t end, const unsigned threadId)
                   {
        int *localCounts = &counts[threadId * BINS];
        float &localSpeed = totalSpeed[threadId];
        float &localKineticEnergy = totalKineticEnergy[threadId];
        float &localAcceleration = totalAcceleration[threadId];
        float &localDensity = totalDensity[threadId];

        for (size_t i = begin; i < end; i++)
        {
            const ParticleObject &p = _particles[i];

            float vx = p.velocity.x;
            float vy = p.velocity.y;

            float ax = p.acceleration.x;
            float ay = p.acceleration.y;

            float speed = std::sqrt(vx * vx + vy * vy);
            float acceleration = std::sqrt(ax * ax + ay * ay);

            localSpeed += speed;
            localKineticEnergy += 0.5f * Globals::particl_mass * speed * speed;
            localAcceleration += acceleration;

// Reuse the densities computed by the latest simulation step instead of
            // running a density sweep once per frame; fall back to a fresh
            // neighbour lookup when the cache is stale
            if (hasDensities)
                localDensity += _densities[i];
            else
                localDensity += _collision.calculateDensity(positions[i], hash);

            int bin = static_cast<int>((speed / MAX_SPEED) * BINS);

            if (bin >= BINS)
                bin = BINS - 1;

            localCounts[bin]++;
        } });

    // Merge all threads' private accumulators (single threaded)
    int mergedCounts[BINS] = {};
    float speedSum = 0.0f;
    float kineticEnergySum = 0.0f;
    float accelerationSum = 0.0f;
    float densitySum = 0.0f;

    for (unsigned threadId = 0; threadId < threads; threadId++)
    {
        for (int bin = 0; bin < BINS; bin++)
            mergedCounts[bin] += counts[threadId * BINS + bin];

        speedSum += totalSpeed[threadId];
        kineticEnergySum += totalKineticEnergy[threadId];
        accelerationSum += totalAcceleration[threadId];
        densitySum += totalDensity[threadId];
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

    return {
        entropy / std::logf(BINS),
        speedSum / divider,
        kineticEnergySum / divider,
        accelerationSum / divider,
        densitySum,
    };
}

void Particle::simulateEarthAndMoon(const int sw, const int sh)
{
    // Earth
    ParticleObject earth;

    earth.position.x = sw / 2.0f;
    earth.position.y = sh / 2.0f;

    earth.velocity.x = 0.0f;
    earth.velocity.y = 0.0f;

    _particles.push_back(earth);

    // Moon
    ParticleObject moon;

    float orbitDistance = sw * 0.30f;

    moon.position.x = sw / 2.0f + orbitDistance;
    moon.position.y = sh / 2.0f;

    moon.velocity.x = 0.0f;
    moon.velocity.y = -std::sqrt(Globals::gravity / orbitDistance);

    _particles.push_back(moon);
}
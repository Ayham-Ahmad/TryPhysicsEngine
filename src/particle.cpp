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

            // Set a random velocity
            p.velocity = Scale({-0.5f + randomFloat(0, 1), -0.5f + randomFloat(0, 1)}, 2);
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

    _fluidHashGrid.initialize(_particles);

    // Force the metrics cache to recompute (count or layout changed)
    _statsTimer = STATS_RATE;
}

void Particle::neighbourSearch(SDL_FPoint mousePos)
{
    _fluidHashGrid.cellSize = Globals::interatctionRadius;

    _fluidHashGrid.clearGrid();
    _fluidHashGrid.mapParticleToCell();

    _hoverPos = mousePos;

    const SDL_FPoint cellIndex = _fluidHashGrid.getGridIndexFromPos(mousePos);
    const int cellX = static_cast<int>(cellIndex.x);
    const int cellY = static_cast<int>(cellIndex.y);

    for (size_t i = 0; i < _particles.size(); i++)
    {
        _particles[i].color = {0.0f, 1.0f, 1.0f, 1.0f};
    }

    // Highlight the 3x3 cells the solver searches: dim orange ring, solid centre
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            const bool centre = (x == 0 && y == 0);
            const auto &content = _fluidHashGrid.getContentOffCell(
                _fluidHashGrid.cellIndexToHash(cellX + x, cellY + y));

            for (size_t i : content)
            {
                _particles[i].color = centre
                                          ? SDL_FColor{1.0f, 0.5f, 0.0f, 1.0f}
                                          : SDL_FColor{0.9f, 0.45f, 0.15f, 1.0f};
            }
        }
    }
}

void Particle::doubleDensityRelaxation(const double simulationDeltaTime, const size_t &count)
{
    for (size_t i = 0; i < count; i++)
    {
        float density = 0;
        float densityNear = 0;
        const auto neighbours = _fluidHashGrid.getNeighbourOfParticleIdx(i);
        ParticleObject &particleA = _particles[i];

        for (size_t j = 0; j < neighbours.size(); j++)
        {
            const size_t neighbourIndex = neighbours[j];
            if (i == neighbourIndex)
                continue;

            const ParticleObject &particleB = _particles[neighbourIndex];

            SDL_FPoint rij = Sub(particleB.position, particleA.position);
            float q = Length(rij) / Globals::interatctionRadius;

            if (q < 1.0f)
            {
                density += pow(1 - q, 2);
                densityNear += pow(1 - q, 3);
            }
        }

        float pressure = Globals::k * (density - Globals::targetDensity);
        float pressureNear = Globals::kNear * densityNear;
        SDL_FPoint particleADisplacement = Zero();

        for (size_t j = 0; j < neighbours.size(); j++)
        {
            const size_t neighbourIndex = neighbours[j];
            if (i == neighbourIndex)
                continue;

            ParticleObject &particleB = _particles[neighbourIndex];

            SDL_FPoint rij = Sub(particleB.position, particleA.position);
            const float length = Length(rij);
            const float q = length / Globals::interatctionRadius;

            if (q < 1.0f && length > 1e-9f)
            {
                rij = Noralize(rij);
                float displacementTrem = pow(simulationDeltaTime, 2) *
                                         (pressure * (1 - q) + pressureNear * pow(1 - q, 2));
                SDL_FPoint D = Scale(rij, displacementTrem);

                particleB.position = Add(particleB.position, Scale(D, 0.5));
                particleADisplacement = Sub(particleADisplacement, Scale(D, 0.5));
            }
        }
        particleA.position = Add(particleA.position, particleADisplacement);
    }
}

void Particle::update(const double simulationDeltaTime, SDL_FPoint mousePos)
{
    const size_t count = _particles.size();

    _positions.clear();

    if (Globals::isGravity)
    {
        for (size_t i = 0; i < count; i++)
        {
            _particles[i].velocity = Add(_particles[i].velocity, Scale({0, Globals::gravity}, simulationDeltaTime));
        }
    }

    // predict positions
    for (size_t i = 0; i < count; i++)
    {
        ParticleObject &particle = _particles[i];
        particle.prevPosition = particle.position;
        SDL_FPoint positionDelta = Scale(particle.velocity, simulationDeltaTime * Globals::coefficientOfRestitution);
        particle.position = Add(particle.position, positionDelta);
        _positions.push_back(particle.position);
    }

    neighbourSearch(mousePos);
    // doubleDensityRelaxation(simulationDeltaTime, count);

    // collision with the screen
    for (size_t i = 0; i < count; i++)
    {
        ParticleObject &particle = _particles[i];
        Physics2D::checkCollisionBetweenObjAndScreen(particle);
    }

    // compute next velocity
    for (size_t i = 0; i < count; i++)
    {
        ParticleObject &particle = _particles[i];
        SDL_FPoint velocity = Scale(Sub(particle.position, particle.prevPosition), 1.0f / simulationDeltaTime);
        particle.velocity = velocity;
    }

    // _spatialHash.update(_positions, Globals::smoothingRadius);
}

void Particle::render(SDL_Renderer *r) const
{
    const size_t count = _particles.size();
    // const bool hasDensities = _densities.size() == count;

    // // Normalize the pressure colors: max |density - targetDensity| over all particles
    // // (skipped entirely while the speed colour ramp is active)
    // float maxAbs = 0.0f;
    // if (hasDensities && !Globals::speedColoring)
    // {
    //     for (size_t i = 0; i < count; i++)
    //         maxAbs = std::max(maxAbs, std::abs(_densities[i] - Globals::targetDensity));
    // }

    // const float invMaxAbs = maxAbs < 1e-12f ? 0.0f : 1.0f / maxAbs;

    // const auto particleColor = [&](const size_t i) -> SDL_FColor
    // {
    //     if (Globals::speedColoring)
    //     {
    //         const ParticleObject &p = _particles[i];
    //         const float speed = std::sqrt(p.velocity.x * p.velocity.x + p.velocity.y * p.velocity.y);
    //         const float speedT = std::min(1.0f, speed / Globals::velocityMax);
    //         return speedRampColor(speedT);
    //     }

    //     const float pressure = hasDensities ? _densities[i] - Globals::targetDensity : 0.0f;
    //     const float t = std::max(-1.0f, std::min(1.0f, pressure * invMaxAbs));

    //     SDL_FColor color;
    //     if (t <= 0.0f)
    //     {
    //         color.r = 1.0f + t;
    //         color.g = 1.0f + t;
    //         color.b = 1.0f;
    //     }
    //     else
    //     {
    //         color.r = 1.0f;
    //         color.g = 1.0f - t;
    //         color.b = 1.0f - t;
    //     }

    //     color.a = 1.0f;
    //     return color;
    // };

    // if (Globals::circles)
    //     Shape::glowField(r, _particles.begin(), _particles.end(), Colors::CyanF(), _glow);

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

            Shape::shape(r, posPx, radius, Globals::PARTICLE_RESOLUTION, p.color);
        }

        // Hash demo overlay: hovered cell, the searched 3x3 block and the radius h
        const float ppu = Globals::pixelsPerMeter;
        const float cellPx = Globals::interatctionRadius * ppu;

        const float cellX = std::floor(_hoverPos.x / Globals::interatctionRadius);
        const float cellY = std::floor(_hoverPos.y / Globals::interatctionRadius);

        const SDL_FRect hoveredCell = {
            cellX * cellPx,
            cellY * cellPx,
            cellPx,
            cellPx};

        const SDL_FRect searchedCells = {
            hoveredCell.x - cellPx,
            hoveredCell.y - cellPx,
            cellPx * 3.0f,
            cellPx * 3.0f};

        SDL_SetRenderDrawColor(r, 0, 255, 0, 40);
        SDL_RenderFillRect(r, &hoveredCell);

        SDL_SetRenderDrawColor(r, 255, 0, 0, 200);
        SDL_RenderRect(r, &searchedCells);
        SDL_SetRenderDrawColor(r, 0, 0, 255, 200);
        SDL_RenderRect(r, &hoveredCell);

        Shape::emptyCircle(
            r,
            static_cast<int16_t>(_hoverPos.x * ppu),
            static_cast<int16_t>(_hoverPos.y * ppu),
            static_cast<int16_t>(Globals::interatctionRadius * ppu));
    }

    // std::pair gridIndexPair =  _spatialHash.positionToCellCoord(_particles[0].position, Globals::smoothingRadius);

    // SDL_FPoint gridIndex = {static_cast<float>(gridIndexPair.first), static_cast<float>(gridIndexPair.second)};

    // uint32_t hash2d = _spatialHash.hashCell(gridIndexPair.first, gridIndexPair.second);
    // uint32_t key = _spatialHash.getKeyFromHash(hash2d);

    // drawText(r, "(" + std::to_string(gridIndexPair.first) + ", " + std::to_string(gridIndexPair.second) + ")", _particles[0].position.x * Globals::pixelsPerMeter, _particles[0].position.y * Globals::pixelsPerMeter + 20, 20);
    // drawText(r, static_cast<int>(hash2d), _particles[0].position.x * Globals::pixelsPerMeter, _particles[0].position.y * Globals::pixelsPerMeter + 45, 20);
    // drawText(r, static_cast<int>(key), _particles[0].position.x * Globals::pixelsPerMeter, _particles[0].position.y * Globals::pixelsPerMeter + 70, 20);

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

std::vector<ParticleObject> &Particle::getParticlesList()
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
#include "particle.h"

void Particle::init(SDL_Renderer *r)
{
    int particlesPerRow = (int)sqrt(Globals::particlesCount);
    int particlesPerCol = (Globals::particlesCount - 1) / particlesPerRow + 1;
    const float particlesSpacing = Globals::particlesSpacing + Globals::particleSize * 2;

    _particles.clear();

    // Create each particle
    for (int i = 0; i < Globals::particlesCount; i++)
    {
        ParticleObject p;

        p.radius = Globals::particleSize;
        p.resolution = Globals::PARTICLE_RESOLUTION;
        p.rigidBody.mass = Globals::particl_mass;

        if (Globals::random)
        {
            // Set a random position
            p.rigidBody.position.x = randomFloat(
                Globals::particleSize,
                Globals::sw - Globals::particleSize);

            p.rigidBody.position.y = randomFloat(
                Globals::particleSize,
                Globals::sh - Globals::particleSize);
        }
        else
        {
            // Calculate the row and column
            float x = Globals::sw / 2.0f +
                      (i % particlesPerRow - particlesPerRow / 2.0f + 0.5f) * particlesSpacing;

            float y = Globals::sh / 2.0f +
                      (i / particlesPerRow - particlesPerCol / 2.0f + 0.5f) * particlesSpacing;

            // Set the particle position
            p.rigidBody.position.x = x;
            p.rigidBody.position.y = y;
        }

        // Set a random color
        // p.color = {randomFloat(0, 1), randomFloat(0, 1), randomFloat(0, 1), 1};
        p.color = _colors.CyanF();

        _particles.push_back(p);
    }
}

void Particle::update(const double simulationDeltaTime)
{
    for (ParticleObject &p : _particles)
    {
        // Set acceleration
        if (Globals::isGravity)
            p.rigidBody.acceleration.y = Globals::gravity;
        else
            p.rigidBody.acceleration.y = 0;

        p.rigidBody.acceleration.x = 0;

        if (Globals::GForce)
        {
            for (ParticleObject &otherPs : _particles)
            {
                if (&p == &otherPs)
                    continue;
                Physics2D::applyGforce(p, otherPs);
            }
        }

        // Update velocity
        p.rigidBody.velocity.y += p.rigidBody.acceleration.y * simulationDeltaTime;
        p.rigidBody.velocity.x += p.rigidBody.acceleration.x * simulationDeltaTime;

        // Update position
        p.rigidBody.position.x += p.rigidBody.velocity.x * simulationDeltaTime;
        p.rigidBody.position.y += p.rigidBody.velocity.y * simulationDeltaTime;

        // Check collision with the screen
        _collision.checkCollisionBwteenObjAndScreen(p);

        // Check collision between particles
        _collision.checkCollisionBetweenObjs(p, _particles);
    }
}

void Particle::render(SDL_Renderer *r) const
{
    // Render each particle
    for (const ParticleObject &p : _particles)
    {
        Shape::shape(r, p.rigidBody.position, p.radius, p.resolution, p.color);
    }

    // Render each Circle
    if (Globals::circles)
    {
        SDL_Texture *texture = getCircleTexture(r);

        for (const ParticleObject &p : _particles)
        {
            const float dstSize = std::min(p.radius * Globals::smoothingRadius, 400.0f);

            SDL_FRect dst = {
                p.rigidBody.position.x - dstSize / 2.0f,
                p.rigidBody.position.y - dstSize / 2.0f,
                dstSize,
                dstSize};

            SDL_RenderTexture(r, texture, nullptr, &dst);
        }
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

// Get the circle texture, regenerating it only when the smoothing radius changes
SDL_Texture *Particle::getCircleTexture(SDL_Renderer *r) const
{
    const int smoothing = static_cast<int>(Globals::smoothingRadius);

    if (circleTexture && circleTextureSmoothing == smoothing)
        return circleTexture;

    if (circleTexture)
        SDL_DestroyTexture(circleTexture);

    circleTexture = Shape::circleTexture(r);
    circleTextureSmoothing = smoothing;

    return circleTexture;
}

const std::vector<ParticleObject> &Particle::getParticlesList() const
{
    return _particles;
}

PhysicsStats Particle::getPhysicsStats() const
{
    static constexpr int BINS = 10;
    static constexpr float MAX_SPEED = 1000.0f;

    int counts[BINS] = {};
    float totalSpeed = 0.0f;
    float totalKineticEnergy = 0.0f;
    float totalAcceleration = 0.0f; 
    float density = 0.0f;

    for (const ParticleObject &p : _particles)
    {
        float vx = p.rigidBody.velocity.x;
        float vy = p.rigidBody.velocity.y;

        float ax = p.rigidBody.acceleration.x;
        float ay = p.rigidBody.acceleration.y;

        float speed = std::sqrt(vx * vx + vy * vy);
        float acceleration = std::sqrt(ax * ax + ay * ay);

        totalSpeed += speed;
        totalKineticEnergy += 0.5f * p.rigidBody.mass * speed * speed;
        totalAcceleration += acceleration;
        density += _collision.calculateDensity(p, _particles);

        int bin = static_cast<int>((speed / MAX_SPEED) * BINS);

        if (bin >= BINS)
            bin = BINS - 1;

        counts[bin]++;
    }

    // Calculate Shannon entropy
    float entropy = 0.0f;

    for (int i = 0; i < BINS; i++)
    {
        if (counts[i] > 0)
        {
            float probability = static_cast<float>(counts[i]) / _particles.size();

            entropy -= probability * std::log(probability);
        }
    }

    const float count = _particles.empty() ? 1.0f : static_cast<float>(_particles.size());

    return {
        entropy / std::logf(BINS),
        totalSpeed / count,
        totalKineticEnergy / count,
        totalAcceleration / count,
        density,
    };
}

void Particle::simulateEarthAndMoon(const int sw, const int sh)
{
    constexpr int PARTICLE_RESOLUTION = 32;

    // Earth
    ParticleObject earth;

    earth.radius = 100;
    earth.resolution = PARTICLE_RESOLUTION;

    earth.rigidBody.position.x = sw / 2.0f;
    earth.rigidBody.position.y = sh / 2.0f;

    SDL_FRect rect1 = {earth.rigidBody.position.x - 100, earth.rigidBody.position.y - 50, 30, 30};
    SDL_FRect rect2 = {earth.rigidBody.position.x + 50, earth.rigidBody.position.y - 80, 40, 40};
    SDL_FRect rect3 = {earth.rigidBody.position.x + 80, earth.rigidBody.position.y + 40, 25, 25};

    earth.rigidBody.velocity.x = 0.0f;
    earth.rigidBody.velocity.y = 0.0f;

    earth.rigidBody.mass = 1000000.0f;

    earth.color = {70 / 255.0f, 130 / 255.0f, 200 / 255.0f, 1};

    _particles.push_back(earth);

    // Moon
    ParticleObject moon;

    moon.radius = Globals::particleSize;
    moon.resolution = PARTICLE_RESOLUTION;

    float orbitDistance = sw * 0.30f;

    moon.rigidBody.position.x = sw / 2.0f + orbitDistance;
    moon.rigidBody.position.y = sh / 2.0f;

    moon.rigidBody.velocity.x = 0.0f;
    moon.rigidBody.velocity.y = -sqrt(Globals::gravity * earth.rigidBody.mass / orbitDistance);

    moon.rigidBody.mass = 1.0f;

    moon.color = {160 / 255.0f, 160 / 255.0f, 160 / 255.0f, 1};

    _particles.push_back(moon);
}
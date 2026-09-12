#include "particle.h"

void Particle::initParticles()
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
        p.resolution = 100;
        p.rigidBody.mass = 100.0f;

        // Calculate the row and column
        float x = Globals::sw / 2.0f + (i % particlesPerRow - particlesPerRow / 2.0f + 0.5f) * particlesSpacing;
        float y = Globals::sh / 2.0f + (i / particlesPerRow - particlesPerCol / 2.0f + 0.5f) * particlesSpacing;

        // Set the particle position
        p.rigidBody.position.x = x;
        p.rigidBody.position.y = y;

        // Set a random color
        // p.color = {randomFloat(0, 1), randomFloat(0, 1), randomFloat(0, 1), 1};
        p.color = _colors.CyanF();

        _particles.push_back(p);
    }
}

void Particle::updateParticles(const double simulationDeltaTime)
{
    for (ParticleObject &p : _particles)
    {
        // Set acceleration
        p.rigidBody.acceleration.y = Globals::gravity;
        p.rigidBody.acceleration.x = 0;

        // Reset acceleration
        // p.rigidBody.acceleration.x = 0;
        // p.rigidBody.acceleration.y = 0;

        for (ParticleObject &otherPs : _particles)
        {
            if (&p == &otherPs)
                continue;
            p.rigidBody.physics2D.applyGforce(p, otherPs);
        }

        // Update velocity
        p.rigidBody.velocity.y += p.rigidBody.acceleration.y * simulationDeltaTime;
        p.rigidBody.velocity.x += p.rigidBody.acceleration.x * simulationDeltaTime;

        // Update position
        p.rigidBody.position.x += p.rigidBody.velocity.x * simulationDeltaTime;
        p.rigidBody.position.y += p.rigidBody.velocity.y * simulationDeltaTime;

        // Check collision with the screen
        _collision.checkCollisionBwteenObjAndScreen(p);
    }

    // Check collision between particles
    _collision.checkCollisionBetweenObjs(_particles);
}

void Particle::renderParticles(SDL_Renderer *r)
{
    // Render each particle
    for (ParticleObject &p : _particles)
    {
        p.s.shape(r, p.rigidBody.position, p.radius, p.resolution, p.color);
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

const std::vector<ParticleObject> &Particle::getParticlesList() const
{
    return _particles;
}

void Particle::simulateEarthAndMoon(const int sw, const int sh)
{
    // Earth
    ParticleObject earth;

    earth.radius = 100;
    earth.resolution = 100;

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
    moon.resolution = 100;

    float orbitDistance = sw * 0.30f;

    moon.rigidBody.position.x = sw / 2.0f + orbitDistance;
    moon.rigidBody.position.y = sh / 2.0f;

    moon.rigidBody.velocity.x = 0.0f;
    moon.rigidBody.velocity.y = -sqrt(Globals::gravity * earth.rigidBody.mass / orbitDistance);

    moon.rigidBody.mass = 1.0f;

    moon.color = {160 / 255.0f, 160 / 255.0f, 160 / 255.0f, 1};

    _particles.push_back(moon);
}
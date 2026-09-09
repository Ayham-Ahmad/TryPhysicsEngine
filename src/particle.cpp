#include "particle.h"

const void Particle::initParticles(
    const int16_t particlesCounter,
    const int sw, const int sh)
{
    _screenwidth = sw;
    _screenHeight = sh;

    // Calculate the starting X position
    _startX = _screenwidth / 2.0f - _spacing * (_particlesPerRow - 1) / 2.0f;

    // Calculate the starting Y position
    _startY = _screenHeight / 2;

    // Create each particle
    for (int i = 0; i < particlesCounter; i++)
    {
        ParticleObject p;

        p.radius = 10;
        p.resolution = 100;

        // Calculate the row and column
        int row = i / _particlesPerRow;
        int col = i % _particlesPerRow;

        // Set the particle position
        p.rigidBody.position.x = _startX + col * _spacing;
        p.rigidBody.position.y = _startY + row * _spacing;

        // Set a random color
        p.color = {randomFloat(0, 1), randomFloat(0, 1), randomFloat(0, 1), 1};

        _particles.push_back(p);
    }
}

const void Particle::updateParticles(float gravity, float resolution)
{
    for (ParticleObject &p : _particles)
    {
        // Set acceleration
        p.rigidBody.acceleration.y = gravity;
        // p.acceleration.x = randomInt(-accelerationRandomNumber, accelerationRandomNumber);
        p.rigidBody.acceleration.x = gravity;

        // Set the particle resolution
        p.resolution = resolution;

        // Update velocity
        p.rigidBody.velocity.y += p.rigidBody.acceleration.y * deltaTime;
        p.rigidBody.velocity.x += p.rigidBody.acceleration.x * deltaTime;

        // Update position
        p.rigidBody.position.x += p.rigidBody.velocity.x * deltaTime;
        p.rigidBody.position.y += p.rigidBody.velocity.y * deltaTime;

        // Check collision with the screen
        _collision.checkCollisionBwteenObjAndScreen(p, _screenHeight, _screenwidth);
    }

    // Check collision between particles
    _collision.checkCollisionBetweenObjs(_particles);
}

const void Particle::renderParticles(SDL_Renderer *r)
{
    // Render each particle
    for (ParticleObject &p : _particles)
    {
        p.s.shape(r, p.rigidBody.position, p.radius, p.resolution, p.color);
    }
}
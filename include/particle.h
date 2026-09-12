#pragma once
#include <SDL3/SDL.h>

#include "shapes.h"
#include "random.h"
#include "physics.h"
#include "colors.h"

struct RigidBody
{
    SDL_FPoint position;
    SDL_FPoint velocity = {0, 0};
    SDL_FPoint acceleration = {0, 0};

    float mass = 1.0f;

    Physics2D physics2D;
};

struct ParticleObject
{
    Shape s;
    float radius;
    float resolution;

    RigidBody rigidBody;

    SDL_FColor color;
    SDL_Rect bounds{};
};

#include "collision.h"

class Particle
{
private:
    // Set particle grid properties
    const int _particlesPerRow = 15;
    int _spacing = 25;
    int _startX, _startY;

    std::vector<ParticleObject> _particles;

    Collision _collision;
    Colors _colors;

public:
    void initParticles();
    void updateParticles(const double simulationDeltaTime);
    void renderParticles(SDL_Renderer *r);
    const std::vector<ParticleObject> &getParticlesList() const;
    void simulateEarthAndMoon(const int sw, const int sh);
};

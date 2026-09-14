#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <vector>

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
};

struct ParticleObject
{
    float radius;
    float resolution;

    RigidBody rigidBody;

    SDL_FColor color;
    SDL_Rect bounds{};
};

#include "collision.h"

struct PhysicsStats
{
    float entropy = 0.0f;
    float speed = 0.0f;
    float kineticEnergy = 0.0f;
    float acceleration = 0.0f;
    float denstiy = 0.0f;
};

class Particle
{
private:
    std::vector<ParticleObject> _particles;
    mutable SDL_Texture *circleTexture = nullptr;
    mutable int circleTextureSmoothing = -1;

    Collision _collision;
    Colors _colors;

    SDL_Texture *getCircleTexture(SDL_Renderer *r) const;

public:
    void init(SDL_Renderer *r);
    void update(const double simulationDeltaTime);
    void render(SDL_Renderer *r) const;
    const std::vector<ParticleObject> &getParticlesList() const;
    PhysicsStats getPhysicsStats() const;
    void simulateEarthAndMoon(const int sw, const int sh);
};
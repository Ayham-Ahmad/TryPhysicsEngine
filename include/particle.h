#pragma once
#include <SDL3/SDL.h>
#include <vector>

#include "shapes.h"
#include "random.h"
#include "physics.h"
#include "colors.h"
#include "parallel.h"
#include "spatialhash.h"

struct ParticleObject
{
    SDL_FPoint position;
    SDL_FPoint velocity = {0, 0};
    SDL_FPoint acceleration = {0, 0};
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

// A single arrow on the gradient field; tip == pos means "draw a dot" (zero magnitude)
struct GridArrow
{
    SDL_FPoint pos;
    SDL_FPoint tip;
};

class Particle
{
private:
    std::vector<ParticleObject> _particles;
    std::vector<float> _densities;
    // Scratch positions (meters) the spatial hash is rebuilt from every step
    std::vector<SDL_FPoint> _positions;
    SpatialHash _spatialHash;
    mutable SDL_Texture *circleTexture = nullptr;
    mutable float circleTextureSmoothing = -1.0f;
    mutable float circleTextureParticleSize = -1.0f;
    mutable float _arrowRefreshTimer = 0.0f;
    mutable std::vector<GridArrow> _arrowCache;

    Collision _collision;

    SDL_Texture *getCircleTexture(SDL_Renderer *r) const;
    void refreshGradientArrows() const;

public:
    ~Particle();

    void init(SDL_Renderer *r);
    void update(const double simulationDeltaTime);
    void render(SDL_Renderer *r) const;
    const std::vector<ParticleObject> &getParticlesList() const;
    PhysicsStats getPhysicsStats() const;
};
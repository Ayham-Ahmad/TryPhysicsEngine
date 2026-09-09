#pragma once
#include <SDL3/SDL.h>

#include "shapes.h"
#include "random.h"

struct ParticleObject
{
    Shape s;
    float radius;
    float resolution;
    float mass = 10.0f;

    SDL_FColor color;

    SDL_FPoint position;
    SDL_FPoint velocity = {0, 0};
    SDL_FPoint acceleration = {0, 0};

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

    int _screenwidth, _screenHeight;

    std::vector<ParticleObject> _particles;

    Collision _collision;

public:
    const void initParticles(const int16_t particlesCounter, const int sw, const int sh);
    const void updateParticles(float gravity, float resolution);
    const void renderParticles(SDL_Renderer *r);
};

#pragma once

#include <SDL3/SDL.h>

// A single simulation particle. The sim runs in meters; rendering converts to
// pixels where needed.
struct ParticleObject
{
    SDL_FPoint position = {0, 0};
    SDL_FPoint velocity = {0, 0};
    SDL_FPoint acceleration = {0, 0};
};

// Aggregate metrics reported to the panel and graph
struct PhysicsStats
{
    float entropy = 0.0f;
    float speed = 0.0f;
    float kineticEnergy = 0.0f;
    float acceleration = 0.0f;
    float density = 0.0f;
};
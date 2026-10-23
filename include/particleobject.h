#pragma once

#include <SDL3/SDL.h>

#include "vector2.h"
struct ParticleObject
{
    SDL_FPoint position = Zero();
    SDL_FPoint prevPosition = Zero();
    SDL_FPoint velocity = Zero();
    SDL_FColor color = {0.0f, 1.0f, 1.0f, 1.0f};
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
#pragma once
#include <SDL3/SDL.h>

#include "shapes.h"

struct Particle {
    Shape s;
    float rad;
    float res;

    SDL_FPoint pos;
};
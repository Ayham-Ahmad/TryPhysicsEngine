#pragma once
#include <SDL3/SDL.h>

#include "Utility.h"

struct Particle {
    Shape s;
    float rad;
    float res;

    SDL_FPoint pos;
};
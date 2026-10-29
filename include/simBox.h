#pragma once
#include <SDL3/SDL.h>
#include <vector>

#include "particle.h"

struct Box
{
    SDL_Rect box;
    Particle particles;
};
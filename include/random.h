#pragma once

#include <SDL3/SDL.h>

inline float randomFloat(const float min, const float max)
{
    return min + SDL_randf() * (max - min);
}
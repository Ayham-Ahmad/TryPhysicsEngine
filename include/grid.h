#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <algorithm>

#include "particle.h"
#include "globals.h"

class Grid
{
public:
    void initialize();
    void update(const std::vector<ParticleObject> &particles);
    void render(SDL_Renderer *r, const bool isStarted);

private:
    std::vector<SDL_FRect> _grid;
    std::vector<int> _cells;
};
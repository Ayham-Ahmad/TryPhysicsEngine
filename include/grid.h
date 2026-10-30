#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <algorithm>

#include "particleobject.h"
#include "globals.h"

class Grid
{
public:
    void init(const SDL_Rect &box);
    void update(const std::vector<ParticleObject> &particles, const SDL_Rect &box);
    void render(SDL_Renderer *r, const bool isStarted) const;

private:
    std::vector<SDL_FRect> _grid;
    std::vector<int> _cells;
    float _cellSize = 0.0f;
};
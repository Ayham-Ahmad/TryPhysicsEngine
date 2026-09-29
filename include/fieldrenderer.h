#pragma once

#include <SDL3/SDL.h>
#include <vector>

#include "particleobject.h"
#include "fluid.h"
#include "spatialhash.h"

struct GridArrow
{
    SDL_FPoint pos;
    SDL_FPoint tip;
};

class FieldRenderer
{
public:
    void update(const std::vector<ParticleObject> &particles, const float deltaTime) const;
    void renderArrows(SDL_Renderer *r) const;

private:
    void refreshArrows(const std::vector<ParticleObject> &particles) const;

    Fluid _fluid;
    mutable float _arrowRefreshTimer = 0.0f;
    mutable std::vector<GridArrow> _arrowCache;
};
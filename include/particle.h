#pragma once
#include <SDL3/SDL.h>
#include <vector>

#include "particleobject.h"
#include "shapes.h"
#include "random.h"
#include "text.h"
#include "physics.h"
#include "colors.h"
#include "vector2.h"
#include "fluidHashGrid.h"
#include "Globals.h"
#include "parallel.h"

class Particle
{
private:
    std::vector<ParticleObject> _particles;
    std::vector<float> _densities;
    std::vector<SDL_FPoint> _positions;
    FluidHashGrid _fluidHashGrid{Globals::interatctionRadius};
    mutable Shape::GlowCache _glow;

    // Cached metrics; recomputed at most ~10Hz (see getPhysicsStats)
    mutable PhysicsStats _cachedStats;
    mutable float _statsTimer = 0.0f;

    SDL_Rect simBox;

public:
    void init(SDL_Renderer *r, const SDL_Rect &simBox);
    void update(const double &simulationDeltaTime, SDL_FPoint mousePos);
    void render(SDL_Renderer *r) const;
    const std::vector<ParticleObject> &getParticlesList() const;
    std::vector<ParticleObject> &getParticlesList();
    // PhysicsStats getPhysicsStats() const;

    void neighbourSearch();
    void doubleDensityRelaxation(const double &simulationDeltaTime, const size_t &count);
    void viscosity(const double &simulationDeltaTime, const size_t &count);
};
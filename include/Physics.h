#pragma once

#include <cmath>
#include <vector>
#include <SDL3/SDL.h>
#include <algorithm>

#include "particleobject.h"
#include "globals.h"
#include "fluidHashGrid.h"

class ParticleObject;

// 2D collision response math used by the contact resolution
class Physics2D
{
public:
    Physics2D() = default;

    // Densities and viscosity borrow the particle list and neighbour grid
    Physics2D(std::vector<ParticleObject> &particles, FluidHashGrid &grid)
        : _particles(&particles), _grid(&grid)
    {
    }

    // Density relaxation (Clavet et al. 2005) and viscosity passes
    void density(const double &simulationDeltaTime);
    void viscosity(const double &simulationDeltaTime);

    static SDL_FPoint differenceVector(const SDL_FPoint &firstPosition, const SDL_FPoint &secondPosition) noexcept;
    static float squaredMagnitude(const SDL_FPoint &vector) noexcept;
    static float distance(const SDL_FPoint &firstPosition, const SDL_FPoint &secondPosition) noexcept;
    static SDL_FPoint direction(const SDL_FPoint &difference, const float distance) noexcept;
    static float getOverlapBetweenTwoObjects(float firstRadius, float secondRadius, float distanceBetweenObjects) noexcept;
    static SDL_FPoint getRelativeVelocityBetweenTwoObjects(const SDL_FPoint &firstVelocity, const SDL_FPoint &secondVelocity) noexcept;
    static float getVelocityAlongCollisionNormal(const SDL_FPoint &relativeVelocity, const SDL_FPoint &collisionNormal) noexcept;
    static float getCollisionImpulseMagnitude(float restitution, float velocityAlongNormal, float firstMass, float secondMass) noexcept;
    static SDL_FPoint getCollisionImpulseVector(const SDL_FPoint &collisionNormal, float impulseMagnitude) noexcept;

    // Check collision with the screen
    static void checkCollisionBetweenObjAndScreen(ParticleObject &p, const SDL_Rect &box);

    // Check collision between particles
    static void checkCollisionBetweenObjs(ParticleObject &sampleParticle, std::vector<ParticleObject> &particles);

private:
    std::vector<ParticleObject> *_particles = nullptr;
    FluidHashGrid *_grid = nullptr;
};
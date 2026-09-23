#pragma once

#include <vector>
#include <SDL3/SDL.h>

class ParticleObject;
class SpatialHash;

class Collision
{
public:
    // Check collision with the screen
    static void checkCollisionBetweenObjAndScreen(ParticleObject &p);

    // Check collision between particles
    static void checkCollisionBetweenObjs(ParticleObject &sampleParticle, std::vector<ParticleObject> &particles);

    float calculateDensity(const SDL_FPoint &samplePointPosition, const SpatialHash &spatialHash) const;
    SDL_FPoint calculatePressureForce(const SDL_FPoint &samplePointPosition, float sampleDensity, const std::vector<SDL_FPoint> &positions, const std::vector<float> &densities, const SpatialHash &spatialHash) const;

private:
    float _convertDensityToPressure(float density) const;
    float _calculateSharedPressure(float densityA, float densityB) const;
};
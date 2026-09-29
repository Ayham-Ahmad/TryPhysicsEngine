#include "fluid.h"
#include "globals.h"
#include "spatialhash.h"

#include <algorithm>
#include <cmath>

void Fluid::_refreshKernelCache() const
{
    const float radius = Globals::smoothingRadius;
    const float mass = Globals::particleMass;

    if (radius == _radius && mass == _mass)
        return;

    _radius = radius;
    _mass = mass;

    const float r4 = radius * radius * radius * radius;

    _inverseVolume = 6.0f / static_cast<float>(Globals::PI * r4);
    _derivativeScale = 12.0f / static_cast<float>(Globals::PI * r4);
}

float Fluid::calculateDensity(const SDL_FPoint &samplePointPosition, const SpatialHash &spatialHash) const
{
    _refreshKernelCache();

    float density = 0;

    spatialHash.forEachPointWithinRadius(samplePointPosition, [&](const size_t index, const float sqrDst)
    {
        const float d = _radius - std::sqrt(sqrDst);
        density += _mass * d * d * _inverseVolume;
    });

    return density;
}

SDL_FPoint Fluid::calculatePressureForce(const SDL_FPoint &samplePointPosition, const float sampleDensity, const std::vector<SDL_FPoint> &positions, const std::vector<float> &densities, const SpatialHash &spatialHash) const
{
    _refreshKernelCache();

    SDL_FPoint pressureForce = {0, 0};

    spatialHash.forEachPointWithinRadius(samplePointPosition, [&](const size_t index, const float sqrDst)
    {
        const float dst = std::sqrt(sqrDst);

        // Skip overlapping particles (the sampling particle itself, at exactly 0)
        if (dst < 1e-4f)
            return;

        const float density = densities[index];
        const float sharedPressure = _calculateSharedPressure(sampleDensity, density);

        const SDL_FPoint &p = positions[index];

        // dir points from the sample point toward the particle, same as the video's
        // offset / dst = (positions[other] - positions[self]) / d
        const float dx = p.x - samplePointPosition.x;
        const float dy = p.y - samplePointPosition.y;
        const SDL_FPoint dir = {dx / dst, dy / dst};

        // slope is negative inside the radius, so the term below pushes the sampling
        // particle away from compressed neighbours (a repulsive pressure force)
        const float slope = (dst - _radius) * _derivativeScale;

        // Video form, verbatim: pressureForce += sharedPressure * dir * slope * mass / ρ_j.
        // The caller divides this whole gradient by the sampling particle's own ρ_i.
        pressureForce.x += sharedPressure * dir.x * slope * _mass / density;
        pressureForce.y += sharedPressure * dir.y * slope * _mass / density;
    });

    return pressureForce;
}

float Fluid::_calculateSharedPressure(const float densityA, const float densityB) const
{
    const float pressureA = _convertDensityToPressure(densityA);
    const float pressureB = _convertDensityToPressure(densityB);

    return (pressureA + pressureB) * 0.5f;
}

float Fluid::_convertDensityToPressure(float density) const
{
    // No max(0,...) clamp: below the target density pressure is negative, pulling
    // particles together (surface tension). This is what keeps the fluid moving
    // even without gravity; a clamp leaves no force once density drops below target.
    float densityError = density - Globals::targetDensity;
    return densityError * Globals::pressureMultiplier;
}
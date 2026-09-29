#pragma once

#include <SDL3/SDL.h>
#include <vector>

class SpatialHash;

// Smooth-particle-hydrodynamics half of the old Collision class. Holds the SPH
// kernels (density sweeps + pressure forces) and caches the radius-derived
// kernel constants that only change when smoothingRadius/mass change (sliders
// are locked while the simulation runs).
class Fluid
{
public:
    float calculateDensity(const SDL_FPoint &samplePointPosition, const SpatialHash &spatialHash) const;

    SDL_FPoint calculatePressureForce(const SDL_FPoint &samplePointPosition, float sampleDensity, const std::vector<SDL_FPoint> &positions, const std::vector<float> &densities, const SpatialHash &spatialHash) const;

private:
    float _convertDensityToPressure(float density) const;
    float _calculateSharedPressure(float densityA, float densityB) const;

    void _refreshKernelCache() const;
    mutable float _radius = -1.0f;
    mutable float _inverseVolume = 0.0f;
    mutable float _derivativeScale = 0.0f;
    mutable float _mass = -1.0f;
};
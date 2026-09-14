#pragma once

#include <vector>
#include <SDL3/SDL.h>

class ParticleObject;

class Collision
{
public:
    // Check collision with the screen
    static void checkCollisionBwteenObjAndScreen(ParticleObject &p);

    // Check collision between particles
    static void checkCollisionBetweenObjs(ParticleObject &sampleParticle, std::vector<ParticleObject> &particles);

    float calculateDensity(const ParticleObject &samplePoint, const std::vector<ParticleObject> &particles) const;

private:
    static float _smoothingKernel(float radius, float dst);
};
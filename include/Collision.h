#pragma once

#include <vector>
#include <SDL3/SDL.h>

#include "particle.h"

class Collision
{
public:
    // Check collision with the screen
    static void checkCollisionBwteenObjAndScreen(ParticleObject &p, float sh, float sw);

    // Check collision between particles
    static void checkCollisionBetweenObjs(std::vector<ParticleObject> &particles);

    // Check if two particles are colliding
    static bool collideTest(ParticleObject &a, ParticleObject &b, SDL_FPoint &difference);
};
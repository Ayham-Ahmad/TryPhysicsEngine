#pragma once

#include <cmath>
#include <SDL3/SDL.h>

#include "particle.h"

class ParticleObject;
class Physics2D
{
public:
    SDL_FPoint differenceVector(const SDL_FPoint &firstPosition, const SDL_FPoint &secondPosition);
    float squaredMagnitude(const SDL_FPoint &vector);
    float distance(const SDL_FPoint &vector);
    SDL_FPoint direction(const SDL_FPoint &difference, const float distance);
    float getOverlapBetweenTwoObjects(float firstRadius, float secondRadius, float distanceBetweenObjects);
    SDL_FPoint getRelativeVelocityBetweenTwoObjects(const SDL_FPoint &firstVelocity, const SDL_FPoint &secondVelocity);
    float getVelocityAlongCollisionNormal(const SDL_FPoint &relativeVelocity, const SDL_FPoint &collisionNormal);
    float getCollisionImpulseMagnitude(float restitution, float velocityAlongNormal, float firstMass, float secondMass);
    SDL_FPoint getCollisionImpulseVector(const SDL_FPoint &collisionNormal, float impulseMagnitude);
    void applyGforce(ParticleObject &a, const ParticleObject &b);
};

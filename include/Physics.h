#pragma once

#include <cmath>
#include <SDL3/SDL.h>

#include "particle.h"

class ParticleObject;
class Physics2D
{
public:
    static SDL_FPoint differenceVector(const SDL_FPoint &firstPosition, const SDL_FPoint &secondPosition) noexcept;
    static float squaredMagnitude(const SDL_FPoint &vector) noexcept;
    static float distance(const SDL_FPoint &firstPosition, const SDL_FPoint &secondPosition) noexcept;
    static SDL_FPoint direction(const SDL_FPoint &difference, const float distance) noexcept;
    static float getOverlapBetweenTwoObjects(float firstRadius, float secondRadius, float distanceBetweenObjects) noexcept;
    static SDL_FPoint getRelativeVelocityBetweenTwoObjects(const SDL_FPoint &firstVelocity, const SDL_FPoint &secondVelocity) noexcept;
    static float getVelocityAlongCollisionNormal(const SDL_FPoint &relativeVelocity, const SDL_FPoint &collisionNormal) noexcept;
    static float getCollisionImpulseMagnitude(float restitution, float velocityAlongNormal, float firstMass, float secondMass) noexcept;
    static SDL_FPoint getCollisionImpulseVector(const SDL_FPoint &collisionNormal, float impulseMagnitude) noexcept;
    static void applyGforce(ParticleObject &a, const ParticleObject &b) noexcept;
};
#pragma once

#include <cmath>
#include <SDL3/SDL.h>

class Physics2D
{
public:
    SDL_FPoint getDifferenceVectorBetweenTwoObjects(const SDL_FPoint &firstPosition, const SDL_FPoint &secondPosition);
    float squaredMagnitude(const SDL_FPoint &vector);
    float magnitude(const SDL_FPoint &vector);
    SDL_FPoint normalize(const SDL_FPoint &vector, float vectorMagnitude);
    float getOverlapBetweenTwoObjects(float firstRadius, float secondRadius, float distanceBetweenObjects);
    SDL_FPoint getRelativeVelocityBetweenTwoObjects(const SDL_FPoint &firstVelocity, const SDL_FPoint &secondVelocity);
    float getVelocityAlongCollisionNormal(const SDL_FPoint &relativeVelocity, const SDL_FPoint &collisionNormal);
    float getCollisionImpulseMagnitude(float restitution, float velocityAlongNormal, float firstMass, float secondMass);
    SDL_FPoint getCollisionImpulseVector(const SDL_FPoint &collisionNormal, float impulseMagnitude);
};

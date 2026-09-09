#include "physics.h"

SDL_FPoint Physics2D::getDifferenceVectorBetweenTwoObjects(const SDL_FPoint &firstPosition, const SDL_FPoint &secondPosition)
{
    return {
        secondPosition.x - firstPosition.x,
        secondPosition.y - firstPosition.y};
}

float Physics2D::squaredMagnitude(const SDL_FPoint &vector)
{
    return vector.x * vector.x + vector.y * vector.y;
}

float Physics2D::magnitude(const SDL_FPoint &vector)
{
    return std::sqrt(squaredMagnitude(vector));
}

SDL_FPoint Physics2D::normalize(const SDL_FPoint &vector, float vectorMagnitude)
{
    return {
        vector.x / vectorMagnitude,
        vector.y / vectorMagnitude};
}

float Physics2D::getOverlapBetweenTwoObjects(float firstRadius, float secondRadius, float distanceBetweenObjects)
{
    return firstRadius + secondRadius - distanceBetweenObjects;
}

SDL_FPoint Physics2D::getRelativeVelocityBetweenTwoObjects(const SDL_FPoint &firstVelocity, const SDL_FPoint &secondVelocity)
{
    return {
        secondVelocity.x - firstVelocity.x,
        secondVelocity.y - firstVelocity.y};
}

float Physics2D::getVelocityAlongCollisionNormal(const SDL_FPoint &relativeVelocity, const SDL_FPoint &collisionNormal)
{
    return relativeVelocity.x * collisionNormal.x +
           relativeVelocity.y * collisionNormal.y;
}

float Physics2D::getCollisionImpulseMagnitude(float restitution, float velocityAlongNormal, float firstMass, float secondMass)
{
    return -((1.0f + restitution) * velocityAlongNormal) /
           ((1.0f / firstMass) + (1.0f / secondMass));
}

SDL_FPoint Physics2D::getCollisionImpulseVector(const SDL_FPoint &collisionNormal, float impulseMagnitude)
{
    return {
        collisionNormal.x * impulseMagnitude,
        collisionNormal.y * impulseMagnitude};
}

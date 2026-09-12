#include "physics.h"

// Calculate the difference between the positions
SDL_FPoint Physics2D::differenceVector(const SDL_FPoint &firstPosition, const SDL_FPoint &secondPosition)
{
    return {
        secondPosition.x - firstPosition.x,
        secondPosition.y - firstPosition.y};
}

// Calculate the squared distance
float Physics2D::squaredMagnitude(const SDL_FPoint &vector)
{
    return vector.x * vector.x + vector.y * vector.y;
}

// Calculate the distance between the particles
float Physics2D::distance(const SDL_FPoint &difference)
{
    return std::sqrt(squaredMagnitude(difference));
}

SDL_FPoint Physics2D::direction(const SDL_FPoint &difference, const float distance)
{
    return {
        difference.x / distance,
        difference.y / distance};
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

void Physics2D::applyGforce(ParticleObject &a, const ParticleObject &b)
{
    SDL_FPoint difference = a.rigidBody.physics2D.differenceVector(
        a.rigidBody.position,
        b.rigidBody.position);

    float distance = a.rigidBody.physics2D.distance(difference);

    if (distance == 0.0f)
        return;

    SDL_FPoint direction = a.rigidBody.physics2D.direction(difference, distance);

    float acc = (Globals::gravity * b.rigidBody.mass) / (distance * distance);

    a.rigidBody.acceleration.x -= acc * direction.x;
    a.rigidBody.acceleration.y -= acc * direction.y;
}

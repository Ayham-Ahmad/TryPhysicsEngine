#include "physics.h"

// Calculate the difference between the positions
SDL_FPoint Physics2D::differenceVector(const SDL_FPoint &firstPosition, const SDL_FPoint &secondPosition) noexcept
{
    return {
        secondPosition.x - firstPosition.x,
        secondPosition.y - firstPosition.y};
}

// Calculate the squared distance
float Physics2D::squaredMagnitude(const SDL_FPoint &vector) noexcept
{
    return vector.x * vector.x + vector.y * vector.y;
}

// Calculate the distance between the particles
float Physics2D::distance(const SDL_FPoint &firstPosition, const SDL_FPoint &secondPosition) noexcept
{
    return std::sqrt(squaredMagnitude(differenceVector(firstPosition, secondPosition)));
}

SDL_FPoint Physics2D::direction(const SDL_FPoint &difference, const float distance) noexcept
{
    return {
        difference.x / distance,
        difference.y / distance};
}

float Physics2D::getOverlapBetweenTwoObjects(float firstRadius, float secondRadius, float distanceBetweenObjects) noexcept
{
    return firstRadius + secondRadius - distanceBetweenObjects;
}

SDL_FPoint Physics2D::getRelativeVelocityBetweenTwoObjects(const SDL_FPoint &firstVelocity, const SDL_FPoint &secondVelocity) noexcept
{
    return {
        secondVelocity.x - firstVelocity.x,
        secondVelocity.y - firstVelocity.y};
}

float Physics2D::getVelocityAlongCollisionNormal(const SDL_FPoint &relativeVelocity, const SDL_FPoint &collisionNormal) noexcept
{
    return relativeVelocity.x * collisionNormal.x +
           relativeVelocity.y * collisionNormal.y;
}

float Physics2D::getCollisionImpulseMagnitude(float restitution, float velocityAlongNormal, float firstMass, float secondMass) noexcept
{
    return -((1.0f + restitution) * velocityAlongNormal) /
           ((1.0f / firstMass) + (1.0f / secondMass));
}

SDL_FPoint Physics2D::getCollisionImpulseVector(const SDL_FPoint &collisionNormal, float impulseMagnitude) noexcept
{
    return {
        collisionNormal.x * impulseMagnitude,
        collisionNormal.y * impulseMagnitude};
}

void Physics2D::applyGforce(ParticleObject &a, const ParticleObject &b) noexcept
{
    SDL_FPoint difference = Physics2D::differenceVector(
        a.rigidBody.position,
        b.rigidBody.position);

    float distance = Physics2D::distance(a.rigidBody.position, b.rigidBody.position);

    if (distance == 0.0f)
        return;

    float minimumDistance = a.radius + b.radius;

    if (distance < minimumDistance)
        distance = minimumDistance;

    SDL_FPoint direction = Physics2D::direction(difference, distance);

    float acc = (Globals::gravity * b.rigidBody.mass) / (distance * distance);

    a.rigidBody.acceleration.x -= acc * direction.x;
    a.rigidBody.acceleration.y -= acc * direction.y;
}
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

// Check collision with the screen
void Physics2D::checkCollisionBetweenObjAndScreen(ParticleObject &p, const SDL_Rect &box)
{
    const float pSize = Globals::particleSize;
    
    const float pixelsPerMeter = Globals::pixelsPerMeter;
    const float left = box.x / pixelsPerMeter;
    const float right = (box.x + box.w) / pixelsPerMeter;
    const float top = box.y / pixelsPerMeter;
    const float bottom = (box.y + box.h) / pixelsPerMeter;

    if (Globals::screenCollision)
    {
        // Vertical collision
        if (p.position.y + pSize > bottom)
        {
            p.position.y = bottom - pSize;

            if (p.velocity.y > 0.0f)
                p.velocity.y *= -Globals::coefficientOfRestitution;
        }
        else if (p.position.y - pSize < top)
        {
            p.position.y = top + pSize;

            if (p.velocity.y < 0.0f)
                p.velocity.y *= -Globals::coefficientOfRestitution;
        }

        // Horizontal collision
        if (p.position.x + pSize > right)
        {
            p.position.x = right - pSize;

            if (p.velocity.x > 0.0f)
                p.velocity.x *= -Globals::coefficientOfRestitution;
        }
        else if (p.position.x - pSize < left)
        {
            p.position.x = left + pSize;

            if (p.velocity.x < 0.0f)
                p.velocity.x *= -Globals::coefficientOfRestitution;
        }
    }
    else
    {
        // Vertical wrapping
        if (p.position.y - pSize > bottom)
            p.position.y = top - pSize;
        else if (p.position.y + pSize < top)
            p.position.y = bottom + pSize;

        // Horizontal wrapping
        if (p.position.x - pSize > right)
            p.position.x = left - pSize;
        else if (p.position.x + pSize < left)
            p.position.x = right + pSize;
    }
}

// Check collision between particles
void Physics2D::checkCollisionBetweenObjs(ParticleObject &sampleParticle, std::vector<ParticleObject> &particles)
{
    for (ParticleObject &p : particles)
    {
        if (&p == &sampleParticle)
            continue;

        // Calculate the difference and squared distance between the positions
        float dx = p.position.x - sampleParticle.position.x;
        float dy = p.position.y - sampleParticle.position.y;

        float radiusSum = Globals::particleSize + Globals::particleSize;
        float squaredDistance = dx * dx + dy * dy;

        if (squaredDistance > radiusSum * radiusSum || squaredDistance == 0.0f)
            continue;

        float distance = std::sqrt(squaredDistance);

        // Calculate the collision normal
        SDL_FPoint normal = {dx / distance, dy / distance};

        // Calculate the overlap
        float overlap = Physics2D::getOverlapBetweenTwoObjects(Globals::particleSize, Globals::particleSize, distance);

        // Move the particles apart
        sampleParticle.position.x -= normal.x * overlap * 0.5f;
        sampleParticle.position.y -= normal.y * overlap * 0.5f;

        p.position.x += normal.x * overlap * 0.5f;
        p.position.y += normal.y * overlap * 0.5f;

        // Calculate relative velocity
        SDL_FPoint relativeVelocity =
            Physics2D::getRelativeVelocityBetweenTwoObjects(
                sampleParticle.velocity,
                p.velocity);

        // Calculate velocity along the collision normal
        float velocityAlongNormal =
            Physics2D::getVelocityAlongCollisionNormal(
                relativeVelocity,
                normal);

        // Objects are already moving away from each other
        if (velocityAlongNormal > 0.0f)
            continue;

        // Calculate the collision impulse
        float impulseMagnitude = Physics2D::getCollisionImpulseMagnitude(
            Globals::coefficientOfRestitution,
            velocityAlongNormal,
            Globals::particleMass,
            Globals::particleMass);

        // Calculate the impulse vector
        SDL_FPoint impulse =
            Physics2D::getCollisionImpulseVector(
                normal,
                impulseMagnitude);

        // Apply the impulse to both particles (mass comes from the global)
        sampleParticle.velocity.x -= impulse.x / Globals::particleMass;
        sampleParticle.velocity.y -= impulse.y / Globals::particleMass;

        // Apply the impulse to the second particle
        p.velocity.x += impulse.x / Globals::particleMass;
        p.velocity.y += impulse.y / Globals::particleMass;
    }
}
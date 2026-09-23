#include "collision.h"
#include "particle.h"
#include "spatialhash.h"

// Check collision with the screen
void Collision::checkCollisionBetweenObjAndScreen(ParticleObject &p)
{
    const float pSize = Globals::particleSize;

    // Sim runs in meters; the screen (pixels) is the world boundary
    const float worldW = Globals::sw / Globals::pixelsPerMeter;
    const float worldH = Globals::sh / Globals::pixelsPerMeter;

    if (Globals::screenCollision)
    {
        // Vertical collision
        if (p.position.y + pSize > worldH)
        {
            p.position.y = worldH - pSize;

            if (p.velocity.y > 0.0f)
                p.velocity.y *= -Globals::coefficientOfRestitution;
        }
        else if (p.position.y - pSize < 0.0f)
        {
            p.position.y = pSize;

            if (p.velocity.y < 0.0f)
                p.velocity.y *= -Globals::coefficientOfRestitution;
        }

        // Horizontal collision
        if (p.position.x + pSize > worldW)
        {
            p.position.x = worldW - pSize;

            if (p.velocity.x > 0.0f)
                p.velocity.x *= -Globals::coefficientOfRestitution;
        }
        else if (p.position.x - pSize < 0.0f)
        {
            p.position.x = pSize;

            if (p.velocity.x < 0.0f)
                p.velocity.x *= -Globals::coefficientOfRestitution;
        }
    }
    else
    {
        // Vertical screen wrapping
        if (p.position.y - pSize > worldH)
            p.position.y = -pSize;
        else if (p.position.y + pSize < 0.0f)
            p.position.y = worldH + pSize;

        // Horizontal screen wrapping
        if (p.position.x - pSize > worldW)
            p.position.x = -pSize;
        else if (p.position.x + pSize < 0.0f)
            p.position.x = worldW + pSize;
    }
}

// Check collision between particles
void Collision::checkCollisionBetweenObjs(ParticleObject &sampleParticle, std::vector<ParticleObject> &particles)
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

float Collision::calculateDensity(const SDL_FPoint &samplePointPosition, const SpatialHash &spatialHash) const
{
    const float radius = Globals::smoothingRadius;
    const float radiusFourth = radius * radius * radius * radius;
    const float inverseVolume = 6.0f / static_cast<float>(Globals::PI * radiusFourth);

    float density = 0;

    // Only the neighbours inside the smoothing radius contribute (kernel is 0 beyond).
    // The kernel only needs the distance, which the hash query already provides.
    spatialHash.forEachPointWithinRadius(samplePointPosition, [&](const size_t index, const float sqrDst)
    {
        const float d = radius - std::sqrt(sqrDst);
        density += Globals::particleMass * d * d * inverseVolume;
    });

    return density;
}

SDL_FPoint Collision::calculatePressureForce(const SDL_FPoint &samplePointPosition, const float sampleDensity, const std::vector<SDL_FPoint> &positions, const std::vector<float> &densities, const SpatialHash &spatialHash) const
{
    SDL_FPoint pressureForce = {0, 0};

    const float radius = Globals::smoothingRadius;
    const float radiusFourth = radius * radius * radius * radius;
    const float derivativeScale = 12.0f / static_cast<float>(Globals::PI * radiusFourth);

    spatialHash.forEachPointWithinRadius(samplePointPosition, [&](const size_t index, const float sqrDst)
    {
        const float dst = std::sqrt(sqrDst);

        // Skip overlapping particles (the sampling particle itself, at exactly 0)
        if (dst < 1e-4f)
            return;

        const float density = densities[index];
        const float sharedPressure = _calculateSharedPressure(sampleDensity, density);

        const SDL_FPoint &p = positions[index];

        // dir points from the sample point toward the particle, same as the video's
        // offset / dst = (positions[other] - positions[self]) / d
        const float dx = p.x - samplePointPosition.x;
        const float dy = p.y - samplePointPosition.y;
        const SDL_FPoint dir = {dx / dst, dy / dst};

        // slope is negative inside the radius, so the term below pushes the sampling
        // particle away from compressed neighbours (a repulsive pressure force)
        const float slope = (dst - radius) * derivativeScale;

        // Video form, verbatim: pressureForce += sharedPressure * dir * slope * mass / ρ_j.
        // The caller divides this whole gradient by the sampling particle's own ρ_i.
        pressureForce.x += -sharedPressure * dir.x * slope * Globals::particleMass / density;
        pressureForce.y += -sharedPressure * dir.y * slope * Globals::particleMass / density;
    });

    return pressureForce;
}

float Collision::_calculateSharedPressure(const float densityA, const float densityB) const
{
    const float pressureA = _convertDensityToPressure(densityA);
    const float pressureB = _convertDensityToPressure(densityB);

    return (pressureA + pressureB) * 0.5f;
}

float Collision::_convertDensityToPressure(float density) const
{
    float densityError = density - Globals::targetDensity;
    float pressure = densityError * Globals::pressureMultiplier;
    return pressure;
}
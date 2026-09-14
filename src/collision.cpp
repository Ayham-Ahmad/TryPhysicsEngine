#include "collision.h"
#include "particle.h"

// Check collision with the screen
void Collision::checkCollisionBwteenObjAndScreen(ParticleObject &p)
{
    if (Globals::screenCollision)
    {
        // Vertical collision
        if (p.rigidBody.position.y + p.radius > Globals::sh)
        {
            p.rigidBody.position.y = Globals::sh - p.radius;

            if (p.rigidBody.velocity.y > 0.0f)
                p.rigidBody.velocity.y *= -Globals::coefficientOfRestitution;
        }
        else if (p.rigidBody.position.y - p.radius < 0.0f)
        {
            p.rigidBody.position.y = p.radius;

            if (p.rigidBody.velocity.y < 0.0f)
                p.rigidBody.velocity.y *= -Globals::coefficientOfRestitution;
        }

        // Horizontal collision
        if (p.rigidBody.position.x + p.radius > Globals::sw)
        {
            p.rigidBody.position.x = Globals::sw - p.radius;

            if (p.rigidBody.velocity.x > 0.0f)
                p.rigidBody.velocity.x *= -Globals::coefficientOfRestitution;
        }
        else if (p.rigidBody.position.x - p.radius < 0.0f)
        {
            p.rigidBody.position.x = p.radius;

            if (p.rigidBody.velocity.x < 0.0f)
                p.rigidBody.velocity.x *= -Globals::coefficientOfRestitution;
        }
    }
    else
    {
        // Vertical screen wrapping
        if (p.rigidBody.position.y - p.radius > Globals::sh)
            p.rigidBody.position.y = -p.radius;
        else if (p.rigidBody.position.y + p.radius < 0.0f)
            p.rigidBody.position.y = Globals::sh + p.radius;

        // Horizontal screen wrapping
        if (p.rigidBody.position.x - p.radius > Globals::sw)
            p.rigidBody.position.x = -p.radius;
        else if (p.rigidBody.position.x + p.radius < 0.0f)
            p.rigidBody.position.x = Globals::sw + p.radius;
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
        float dx = p.rigidBody.position.x - sampleParticle.rigidBody.position.x;
        float dy = p.rigidBody.position.y - sampleParticle.rigidBody.position.y;

        float radiusSum = sampleParticle.radius + p.radius;
        float squaredDistance = dx * dx + dy * dy;

        if (squaredDistance > radiusSum * radiusSum || squaredDistance == 0.0f)
            continue;

        float distance = std::sqrt(squaredDistance);

        // Calculate the collision normal
        SDL_FPoint normal = {dx / distance, dy / distance};

        // Calculate the overlap
        float overlap = Physics2D::getOverlapBetweenTwoObjects(sampleParticle.radius, p.radius, distance);

        // Move the particles apart
        sampleParticle.rigidBody.position.x -= normal.x * overlap * 0.5f;
        sampleParticle.rigidBody.position.y -= normal.y * overlap * 0.5f;

        p.rigidBody.position.x += normal.x * overlap * 0.5f;
        p.rigidBody.position.y += normal.y * overlap * 0.5f;

        // Calculate relative velocity
        SDL_FPoint relativeVelocity =
            Physics2D::getRelativeVelocityBetweenTwoObjects(
                sampleParticle.rigidBody.velocity,
                p.rigidBody.velocity);

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
            sampleParticle.rigidBody.mass,
            p.rigidBody.mass);

        // Calculate the impulse vector
        SDL_FPoint impulse =
            Physics2D::getCollisionImpulseVector(
                normal,
                impulseMagnitude);

        // Apply the impulse to the first particle
        sampleParticle.rigidBody.velocity.x -= impulse.x / sampleParticle.rigidBody.mass;
        sampleParticle.rigidBody.velocity.y -= impulse.y / sampleParticle.rigidBody.mass;

        // Apply the impulse to the second particle
        p.rigidBody.velocity.x += impulse.x / p.rigidBody.mass;
        p.rigidBody.velocity.y += impulse.y / p.rigidBody.mass;
    }
}

float Collision::_smoothingKernel(float radius, float dst)
{
    float volume = Globals::PI * std::pow(radius, 8) / 4;
    float value = std::max(0.0f, radius * radius - dst * dst);
    return value * value * value  / volume;
}

float Collision::calculateDensity(const ParticleObject &samplePoint, const std::vector<ParticleObject> &particles) const
{
    float density = 0;

    for (const ParticleObject &p : particles)
    {
        float dst = Physics2D::distance(samplePoint.rigidBody.position, p.rigidBody.position);
        float influence = _smoothingKernel(Globals::smoothingRadius, dst);
        density += p.rigidBody.mass * influence;
    }

    return density;
}
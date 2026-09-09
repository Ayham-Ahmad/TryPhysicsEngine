#include "collision.h"

// Check collision with the screen
void Collision::checkCollisionBwteenObjAndScreen(ParticleObject &p, float sh, float sw)
{
    // Vertical collision
    if (p.rigidBody.position.y + p.radius > sh)
    {
        p.rigidBody.position.y = sh - p.radius;

        if (p.rigidBody.velocity.y > 0.0f)
            p.rigidBody.velocity.y *= -coefficientOfRestitution;
    }
    else if (p.rigidBody.position.y - p.radius < 0.0f)
    {
        p.rigidBody.position.y = p.radius;

        if (p.rigidBody.velocity.y < 0.0f)
            p.rigidBody.velocity.y *= -coefficientOfRestitution;
    }

    // Horizontal collision
    if (p.rigidBody.position.x + p.radius > sw)
    {
        p.rigidBody.position.x = sw - p.radius;

        if (p.rigidBody.velocity.x > 0.0f)
            p.rigidBody.velocity.x *= -coefficientOfRestitution;
    }
    else if (p.rigidBody.position.x - p.radius < 0.0f)
    {
        p.rigidBody.position.x = p.radius;

        if (p.rigidBody.velocity.x < 0.0f)
            p.rigidBody.velocity.x *= -coefficientOfRestitution;
    }
}

// Check collision between particles
void Collision::checkCollisionBetweenObjs(std::vector<ParticleObject> &particles)
{
    // Check every pair of particles
    for (size_t i = 0; i < particles.size(); ++i)
    {
        for (size_t j = i + 1; j < particles.size(); ++j)
        {
            ParticleObject &a = particles[i];
            ParticleObject &b = particles[j];

            // Calculate the difference between the positions
            SDL_FPoint difference =
                a.rigidBody.physics2D.getDifferenceVectorBetweenTwoObjects(
                    a.rigidBody.position,
                    b.rigidBody.position);

            // Check if the particles are colliding
            if (!Collision::collideTest(a, b, difference))
                continue;

            // Calculate the distance between the particles
            float distance = a.rigidBody.physics2D.magnitude(difference);

            // Avoid division by zero
            if (distance == 0.0f)
                continue;

            // Calculate the collision normal
            SDL_FPoint normal = a.rigidBody.physics2D.normalize(difference, distance);

            // Calculate the overlap
            float overlap = a.rigidBody.physics2D.getOverlapBetweenTwoObjects(a.radius, b.radius, distance);

            // Move the particles apart
            a.rigidBody.position.x -= normal.x * overlap * 0.5f;
            a.rigidBody.position.y -= normal.y * overlap * 0.5f;

            b.rigidBody.position.x += normal.x * overlap * 0.5f;
            b.rigidBody.position.y += normal.y * overlap * 0.5f;

            // Calculate relative velocity
            SDL_FPoint relativeVelocity =
                a.rigidBody.physics2D.getRelativeVelocityBetweenTwoObjects(
                    a.rigidBody.velocity,
                    b.rigidBody.velocity);

            // Calculate velocity along the collision normal
            float velocityAlongNormal =
                a.rigidBody.physics2D.getVelocityAlongCollisionNormal(
                    relativeVelocity,
                    normal);

            // Objects are already moving away from each other
            if (velocityAlongNormal > 0.0f)
                continue;

            // Calculate the collision impulse
            float impulseMagnitude = a.rigidBody.physics2D.getCollisionImpulseMagnitude(
                coefficientOfRestitution,
                velocityAlongNormal,
                a.rigidBody.mass,
                b.rigidBody.mass);

            // Calculate the impulse vector
            SDL_FPoint impulse =
                a.rigidBody.physics2D.getCollisionImpulseVector(
                    normal,
                    impulseMagnitude);

            // Apply the impulse to the first particle
            a.rigidBody.velocity.x -= impulse.x / a.rigidBody.mass;
            a.rigidBody.velocity.y -= impulse.y / a.rigidBody.mass;

            // Apply the impulse to the second particle
            b.rigidBody.velocity.x += impulse.x / b.rigidBody.mass;
            b.rigidBody.velocity.y += impulse.y / b.rigidBody.mass;
        }
    }
}

// Check if two particles are colliding
bool Collision::collideTest(
    ParticleObject &a,
    ParticleObject &b,
    SDL_FPoint &difference)
{
    // Calculate the squared distance
    float distanceSquared =
        a.rigidBody.physics2D.squaredMagnitude(difference);

    // Calculate the sum of the radii
    float radiusSum = a.radius + b.radius;

    // Check if the particles overlap
    return distanceSquared <= radiusSum * radiusSum;
}
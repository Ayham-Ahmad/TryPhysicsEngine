#include "collision.h"

// Check collision with the screen
void Collision::collide(ParticleObject &p, float sh, float sw)
{
    // Vertical collision
    if (p.position.y + p.radius > sh)
    {
        p.position.y = sh - p.radius;

        if (p.velocity.y > 0.0f)
            p.velocity.y *= -coefficientOfRestitution;
    }
    else if (p.position.y - p.radius < 0.0f)
    {
        p.position.y = p.radius;

        if (p.velocity.y < 0.0f)
            p.velocity.y *= -coefficientOfRestitution;
    }

    // Horizontal collision
    if (p.position.x + p.radius > sw)
    {
        p.position.x = sw - p.radius;

        if (p.velocity.x > 0.0f)
            p.velocity.x *= -coefficientOfRestitution;
    }
    else if (p.position.x - p.radius < 0.0f)
    {
        p.position.x = p.radius;

        if (p.velocity.x < 0.0f)
            p.velocity.x *= -coefficientOfRestitution;
    }
}

// Check collision between particles
void Collision::objCollide(
    std::vector<ParticleObject> &particles)
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
                {
                    b.position.x - a.position.x,
                    b.position.y - a.position.y};

            // Check if the particles are colliding
            if (!Collision::collideTest(a, b, difference))
                continue;

            // Calculate the distance between the particles
            float distance = std::sqrt(
                difference.x * difference.x +
                difference.y * difference.y);

            // Avoid division by zero
            if (distance == 0.0f)
                continue;

            // Calculate the collision normal
            SDL_FPoint normal =
                {
                    difference.x / distance,
                    difference.y / distance};

            // Calculate the overlap
            float overlap = (a.radius + b.radius) - distance;

            // Move the particles apart
            a.position.x -= normal.x * overlap * 0.5f;
            a.position.y -= normal.y * overlap * 0.5f;

            b.position.x += normal.x * overlap * 0.5f;
            b.position.y += normal.y * overlap * 0.5f;

            // Calculate relative velocity
            SDL_FPoint relativeVelocity =
                {
                    b.velocity.x - a.velocity.x,
                    b.velocity.y - a.velocity.y};

            // Calculate velocity along the collision normal
            float velocityAlongNormal =
                relativeVelocity.x * normal.x +
                relativeVelocity.y * normal.y;

            // Objects are already moving away from each other
            if (velocityAlongNormal > 0.0f)
                continue;

            // Calculate the collision impulse
            float impulseMagnitude =
                -((1.0f + coefficientOfRestitution) *
                  velocityAlongNormal) /
                ((1.0f / a.mass) + (1.0f / b.mass));

            // Calculate the impulse vector
            SDL_FPoint impulse =
                {
                    normal.x * impulseMagnitude,
                    normal.y * impulseMagnitude};

            // Apply the impulse to the first particle
            a.velocity.x -= impulse.x / a.mass;
            a.velocity.y -= impulse.y / a.mass;

            // Apply the impulse to the second particle
            b.velocity.x += impulse.x / b.mass;
            b.velocity.y += impulse.y / b.mass;
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
        difference.x * difference.x +
        difference.y * difference.y;

    // Calculate the sum of the radii
    float radiusSum = a.radius + b.radius;

    // Check if the particles overlap
    return distanceSquared <= radiusSum * radiusSum;
}
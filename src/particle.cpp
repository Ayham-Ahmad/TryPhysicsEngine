#include "particle.h"
#include "Perf.h"

void Particle::init(SDL_Renderer *r, const SDL_Rect &box)
{
    // rows and cols for grid of particles
    int particlesPerRow = (int)sqrt(Globals::particlesCount);
    int particlesPerCol = (Globals::particlesCount - 1) / particlesPerRow + 1;
    const float particlesSpacing = Globals::particlesSpacing + Globals::particleSize * 2;
    const float pixelsPerMeter = Globals::pixelsPerMeter;
    const float left = box.x / pixelsPerMeter;
    const float top = box.y / pixelsPerMeter;
    const float boxWidth = box.w / pixelsPerMeter;
    const float boxHeight = box.h / pixelsPerMeter;

    _particles.clear();
    _springs.clear();
    simBox = box;

    // Create each particle
    for (int i = 0; i < Globals::particlesCount; i++)
    {
        ParticleObject p;

        if (Globals::random)
        {
            // Set a random position
            p.position.x = randomFloat(
                left + Globals::particleSize,
                left + boxWidth - Globals::particleSize);

            p.position.y = randomFloat(
                top + Globals::particleSize,
                top + boxHeight - Globals::particleSize);

            // Set a random velocity
            // p.velocity = Scale({-0.5f + randomFloat(0, 1), -0.5f + randomFloat(0, 1)}, 2);
        }
        else
        {
            // Calculate the row and column
            float x = left + boxWidth / 2.0f +
                      (i % particlesPerRow - particlesPerRow / 2.0f + 0.5f) * particlesSpacing;

            float y = top + boxHeight / 2.0f +
                      (i / particlesPerRow - particlesPerCol / 2.0f + 0.5f) * particlesSpacing;

            // Set the particle position
            p.position.x = x;
            p.position.y = y;
        }

        _particles.push_back(p);
    }

    _fluidHashGrid.initialize(_particles);
}

void Particle::update(const double &simulationDeltaTime, SDL_FPoint mousePos)
{
    const size_t count = _particles.size();

    // _positions.clear();

    if (Globals::isGravity)
    {
        PERF_SCOPE("SPH::applyGravity");
        parallel_for(0, count, [&](int i)
                     { _particles[i].velocity = Add(_particles[i].velocity, Scale({0, Globals::gravity}, (float)simulationDeltaTime)); });
    }

    {
        PERF_SCOPE("SPH::viscosity");
        _physics.viscosity(simulationDeltaTime);
    }

    // predict positions
    {
        PERF_SCOPE("SPH::predict");
        parallel_for(0, count, [&](int i)
                     {
                         ParticleObject &particle = _particles[i];
                         particle.prevPosition = particle.position;
                         SDL_FPoint positionDelta = Scale(particle.velocity, (float)simulationDeltaTime * Globals::coefficientOfRestitution);
                         particle.position = Add(particle.position, positionDelta);
                         // _positions.push_back(particle.position);
                     });
    }

    {
        PERF_SCOPE("SPH::neighbourSearch");
        _fluidHashGrid.neighbourSearch();
    }
    {
        PERF_SCOPE("SPH::density");
        _physics.density(simulationDeltaTime);
    }
    {
        PERF_SCOPE("SPH::springs");
        _springs.update(simulationDeltaTime);
    }

    // collision with the screen
    {
        PERF_SCOPE("SPH::screenCollision");
        parallel_for(0, count, [&](int i)
                     { Physics2D::checkCollisionBetweenObjAndScreen(_particles[i], simBox); });
    }

    // compute next velocity
    {
        PERF_SCOPE("SPH::integrateVelocity");
        parallel_for(0, count, [&](int i)
                     {
            ParticleObject &particle = _particles[i];
            SDL_FPoint velocity = Scale(Sub(particle.position, particle.prevPosition), 1.0f / (float)simulationDeltaTime);
            particle.velocity = velocity; });
    }
}

void Particle::render(SDL_Renderer *r) const
{
    const SDL_FColor defaultColor = {0.1f, 0.1f, 0.9f, 1.0f};

    if (Globals::circles)
        Shape::glowField(r, _particles.begin(), _particles.end(), [](size_t)
                         { return Colors::CyanF(); }, _glow);

    if (Globals::particles)
    {
        const float radius = Globals::particleSize * Globals::pixelsPerMeter;
        Shape::shapesBatch(
            r, _particles.begin(), _particles.end(),
            radius, Globals::PARTICLE_RESOLUTION,
            [&](size_t i)
            {
                return Globals::speedColoring
                           ? Colors::particleColor(_particles[i], 2.0f)
                           : defaultColor;
            });
    }
}

const std::vector<ParticleObject> &Particle::getParticlesList() const
{
    return _particles;
}

std::vector<ParticleObject> &Particle::getParticlesList()
{
    return _particles;
}

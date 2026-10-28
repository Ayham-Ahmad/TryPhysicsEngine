#include "particle.h"

void Particle::init(SDL_Renderer *r)
{
    // rows and cols for grid of particles
    int particlesPerRow = (int)sqrt(Globals::particlesCount);
    int particlesPerCol = (Globals::particlesCount - 1) / particlesPerRow + 1;
    const float particlesSpacing = Globals::particlesSpacing + Globals::particleSize * 2;

    _particles.clear();

    // Create each particle
    for (int i = 0; i < Globals::particlesCount; i++)
    {
        ParticleObject p;

        if (Globals::random)
        {
            // Set a random position
            p.position.x = randomFloat(
                Globals::particleSize,
                Globals::worldW - Globals::particleSize);

            p.position.y = randomFloat(
                Globals::particleSize,
                Globals::worldH - Globals::particleSize);

            // Set a random velocity
            p.velocity = Scale({-0.5f + randomFloat(0, 1), -0.5f + randomFloat(0, 1)}, 2);
        }
        else
        {
            // Calculate the row and column
            float x = Globals::worldW / 2.0f +
                      (i % particlesPerRow - particlesPerRow / 2.0f + 0.5f) * particlesSpacing;

            float y = Globals::worldH / 2.0f +
                      (i / particlesPerRow - particlesPerCol / 2.0f + 0.5f) * particlesSpacing;

            // Set the particle position
            p.position.x = x;
            p.position.y = y;
        }

        _particles.push_back(p);
    }

    _fluidHashGrid.initialize(_particles);
}

void Particle::neighbourSearch()
{
    _fluidHashGrid.cellSize = Globals::interatctionRadius;

    _fluidHashGrid.clearGrid();
    _fluidHashGrid.mapParticleToCell();
}

void Particle::doubleDensityRelaxation(const double &simulationDeltaTime, const size_t &count)
{
    for (size_t i = 0; i < count; i++)
    {
        float density = 0;
        float densityNear = 0;
        const auto neighbours = _fluidHashGrid.getNeighbourOfParticleIdx(i);
        SDL_FPoint &positionA = _particles[i].position;

        for (size_t j = 0; j < neighbours.size(); j++)
        {
            const size_t neighbourIndex = neighbours[j];
            if (i == neighbourIndex)
                continue;

            const SDL_FPoint &positionB = _particles[neighbourIndex].position;

            SDL_FPoint rij = Sub(positionB, positionA);
            float q = Length(rij) / Globals::interatctionRadius;

            if (q < 1.0f)
            {
                density += pow(1 - q, 2);
                densityNear += pow(1 - q, 3);
            }
        }

        float pressure = Globals::k * (density - Globals::targetDensity);
        float pressureNear = Globals::kNear * densityNear;
        SDL_FPoint particleADisplacement = Zero();

        for (size_t j = 0; j < neighbours.size(); j++)
        {
            const size_t neighbourIndex = neighbours[j];
            if (i == neighbourIndex)
                continue;

            SDL_FPoint &positionB = _particles[neighbourIndex].position;

            SDL_FPoint rij = Sub(positionB, positionA);
            const float length = Length(rij);
            const float q = length / Globals::interatctionRadius;

            if (q < 1.0f && length > 1e-9f)
            {
                rij = Normalize(rij);
                float displacementTrem = pow(simulationDeltaTime, 2) *
                                         (pressure * (1 - q) + pressureNear * pow(1 - q, 2));
                SDL_FPoint D = Scale(rij, displacementTrem);

                positionB = Add(positionB, Scale(D, 0.5));
                particleADisplacement = Sub(particleADisplacement, Scale(D, 0.5));
            }
        }
        positionA = Add(positionA, particleADisplacement);
    }
}

void Particle::viscosity(const double &simulationDeltaTime, const size_t &count)
{
    for (size_t i = 0; i < count; i++)
    {
        const auto neighbours = _fluidHashGrid.getNeighbourOfParticleIdx(i);
        ParticleObject &particleA = _particles[i];

        for (size_t j = 0; j < neighbours.size(); j++)
        {
            const size_t neighbourIndex = neighbours[j];
            if (i == neighbourIndex)
                continue;

            ParticleObject &particleB = _particles[neighbourIndex];

            SDL_FPoint rij = Sub(particleA.position, particleB.position);
            float q = Length(rij) / Globals::interatctionRadius;

            if (q < 1.0f)
            {
                rij = Normalize(rij);
                float u = Dot(Sub(particleA.velocity, particleB.velocity), rij);

                if (u > 0)
                {
                    float ITerm = simulationDeltaTime * (1 - q) *
                                  (Globals::Sigma * u + Globals::Beta * u * u);
                    SDL_FPoint I = Scale(rij, ITerm);

                    particleA.velocity = Sub(particleA.velocity, Scale(I, 0.5));
                    particleB.velocity = Add(particleB.velocity, Scale(I, 0.5));
                }
            }
        }
    }
}

void Particle::update(const double &simulationDeltaTime, SDL_FPoint mousePos)
{
    const size_t count = _particles.size();

    _positions.clear();

    if (Globals::isGravity)
    {
        for (size_t i = 0; i < count; i++)
        {
            _particles[i].velocity = Add(_particles[i].velocity, Scale({0, Globals::gravity}, simulationDeltaTime));
        }
    }

    viscosity(simulationDeltaTime, count);

    // predict positions
    for (size_t i = 0; i < count; i++)
    {
        ParticleObject &particle = _particles[i];
        particle.prevPosition = particle.position;
        SDL_FPoint positionDelta = Scale(particle.velocity, simulationDeltaTime * Globals::coefficientOfRestitution);
        particle.position = Add(particle.position, positionDelta);
        _positions.push_back(particle.position);
    }

    neighbourSearch();
    doubleDensityRelaxation(simulationDeltaTime, count);

    // collision with the screen
    for (size_t i = 0; i < count; i++)
    {
        ParticleObject &particle = _particles[i];
        Physics2D::checkCollisionBetweenObjAndScreen(particle);
    }

    // compute next velocity
    for (size_t i = 0; i < count; i++)
    {
        ParticleObject &particle = _particles[i];
        SDL_FPoint velocity = Scale(Sub(particle.position, particle.prevPosition), 1.0f / simulationDeltaTime);
        particle.velocity = velocity;
    }
}

void Particle::render(SDL_Renderer *r) const
{
    const size_t count = _particles.size();
    SDL_FColor color = {0.1f, 0.1f, 0.9f, 1.0f};

    if (Globals::circles)
        Shape::glowField(r, _particles.begin(), _particles.end(), [](size_t)
                         { return Colors::CyanF(); }, _glow);

    // Render each particle
    if (Globals::particles)
    {
        const float radius = Globals::particleSize * Globals::pixelsPerMeter;
        for (size_t i = 0; i < count; i++)
        {
            const ParticleObject &p = _particles[i];

            const SDL_FPoint posPx = {
                p.position.x * Globals::pixelsPerMeter,
                p.position.y * Globals::pixelsPerMeter};

            if (Globals::speedColoring)
                color = Colors::particleColor(p, 2.0f);
            Shape::shape(r, posPx, radius, Globals::PARTICLE_RESOLUTION, color);
        }
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
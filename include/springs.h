#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "Perf.h"
#include "Globals.h"
#include "particleobject.h"
#include "fluidHashGrid.h"
#include "vector2.h"

// Every spring between two particles that are inside the interaction radius,
// plus the passes that create and solve them.
class Spring
{
public:
    // One spring: two particles and the length it wants to be.
    struct Pair
    {
        void init(const size_t &pAIdx, const size_t &pBIdx, const float &length)
        {
            particleA = pAIdx;
            particleB = pBIdx;
            restLength = length;
        }

        size_t particleA = 0;
        size_t particleB = 0;
        float restLength = 0.0f;
    };

    Spring(std::vector<ParticleObject> &particles, FluidHashGrid &grid)
        : _particles(&particles), _grid(&grid)
    {
    }

    void update(const double &simulationDeltaTime)
    {
        const float dt = static_cast<float>(simulationDeltaTime);
        create();
        plasticity(dt);
        displacement(dt);
    }

    void clear() { _springs.clear(); }

    size_t count() const { return _springs.size(); }

private:
    // Builds a spring for every pair of particles that are inside the interaction radius.
    void create()
    {
        PERF_SCOPE("SPH::springs::create");

        const size_t count = _particles->size();

        for (size_t i = 0; i < count; i++)
        {
            const auto &neighbours = _grid->getNeighbourOfParticleIdx(i);
            const SDL_FPoint &positionA = (*_particles)[i].position;

            for (size_t j = 0; j < neighbours.size(); j++)
            {
                const size_t neighbourIndex = neighbours[j];
                if (neighbourIndex <= i)
                    continue;

                // Stable id that does not depend on the particle count (emitters grow it)
                const uint64_t springId = (static_cast<uint64_t>(i) << 32) | static_cast<uint32_t>(neighbourIndex);
                if (_springs.find(springId) != _springs.end())
                    continue;

                const SDL_FPoint rij = Sub((*_particles)[neighbourIndex].position, positionA);
                const float q = Length(rij) / Globals::interatctionRadius;

                if (q < 1.0f)
                {
                    Pair spring;
                    spring.init(i, neighbourIndex, Globals::interatctionRadius);
                    _springs.emplace(springId, spring);
                }
            }
        }
    }

    // Lets every spring's rest length follow its current length.
    void plasticity(const float &dt)
    {
        PERF_SCOPE("SPH::springs::plasticity");

        for (auto it = _springs.begin(); it != _springs.end();)
        {
            Pair &spring = it->second;

            const SDL_FPoint &positionA = (*_particles)[spring.particleA].position;
            const SDL_FPoint &positionB = (*_particles)[spring.particleB].position;

            const float rij = Length(Sub(positionA, positionB));
            const float Lij = spring.restLength;
            const float d = Globals::gamma * Lij;

            if (rij > Lij + d)
            {
                // stretching
                spring.restLength += dt * Globals::plasticity * (rij - Lij - d);
            }
            else if (rij < Lij - d)
            {
                // compression
                spring.restLength -= dt * Globals::plasticity * (Lij - d - rij);
            }

            // A spring can never rest beyond the interaction radius: drop it instead,
            // otherwise it keeps pulling particles that are nowhere near each other.
            if (spring.restLength > Globals::interatctionRadius)
            {
                it = _springs.erase(it);
                continue;
            }

            ++it;
        }
    }

    // Pushes/pulls the two ends of every spring towards its rest length.
    void displacement(const float &dt)
    {
        PERF_SCOPE("SPH::springs::displacement");

        const float dtSquared = dt * dt;

        for (auto &entry : _springs)
        {
            Pair &spring = entry.second;

            SDL_FPoint &positionA = (*_particles)[spring.particleA].position;
            SDL_FPoint &positionB = (*_particles)[spring.particleB].position;

            SDL_FPoint rij = Sub(positionA, positionB);
            const float distance = Length(rij);

            if (distance < 0.0001f)
                continue;

            rij = Normalize(rij);

            const float displacementTerm = dtSquared * Globals::springStiffness *
                                           (1.0f - spring.restLength / Globals::interatctionRadius) *
                                           (spring.restLength - distance);

            rij = Scale(rij, displacementTerm * 0.5f);

            positionA = Add(positionA, rij);
            positionB = Sub(positionB, rij);
        }
    }

    std::unordered_map<uint64_t, Pair> _springs;
    std::vector<ParticleObject> *_particles = nullptr;
    FluidHashGrid *_grid = nullptr;
};

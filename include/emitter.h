#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdlib>

#include "vector2.h"
#include "particleobject.h"
#include "colors.h"
#include "shapes.h"
#include "Globals.h"

class ParticleEmitter
{
private:
    std::vector<ParticleEmitterObject> _emitters;

public:
    void init()
    {
        _emitters.clear();

        const float ppu = Globals::pixelsPerMeter;
        // SDL_FPoint pos1 = {300.0f / ppu, 300.0f / ppu};
        SDL_FPoint pos2 = {900.0f / ppu, 300.0f / ppu};
        // SDL_FPoint pos3 = {1500.0f / ppu, 300.0f / ppu};
        // { position, direction, size, spawnInterval, amount, velocity, timer }
        // ParticleEmitterObject emitter1 = {pos1, {0, -1}, 0.08f, 0.01f, 3, 0.8f, 0.0f};
        ParticleEmitterObject emitter2 = {pos2, {0, -1}, 0.08f, 0.01f, 3, 0.8f, 0.0f};
        // ParticleEmitterObject emitter3 = {pos3, {0, -1}, 0.08f, 0.01f, 3, 0.8f, 0.0f};
        // _emitters.push_back(emitter1);
        _emitters.push_back(emitter2);
        // _emitters.push_back(emitter3);
    }

private:
    void move(const SDL_FPoint &delta)
    {
        for (size_t i = 0; i < _emitters.size(); i++)
        {
            _emitters[i].position = Add(_emitters[i].position, delta);
        }
    }

    void spawn(const double &simulationDeltaTime, std::vector<ParticleObject> &particles)
    {
        for (size_t eIdx = 0; eIdx < _emitters.size(); eIdx++)
        {
            ParticleEmitterObject &e = _emitters[eIdx];

            if (e.amount <= 0 || e.spawnInterval <= 0 || e.size < 0)
                continue;

            const float dirLen = Length(e.direction);
            if (dirLen < 1e-8f)
                continue;

            SDL_FPoint normDir = {e.direction.x / dirLen, e.direction.y / dirLen};
            SDL_FPoint normal = GetNormal(normDir);
            const float normalLen = Length(normal);
            if (normalLen < 1e-8f)
                continue;
            normal = {normal.x / normalLen, normal.y / normalLen};

            e.timer += simulationDeltaTime;
            if (e.timer < e.spawnInterval)
                continue;
            e.timer -= e.spawnInterval;

            SDL_FPoint plane = Scale(normal, -e.size);
            SDL_FPoint planeStart = Add(e.position, plane);
            SDL_FPoint planeEnd = Sub(e.position, plane);
            SDL_FPoint planeVec = Sub(planeEnd, planeStart);

            for (int j = 0; j < static_cast<int>(e.amount); j++)
            {
                float t = static_cast<float>(j) / (e.amount - 1.0f > 0 ? e.amount - 1.0f : 1.0f);
                SDL_FPoint position = Add(planeStart, Scale(planeVec, t));

                ParticleObject newParticle;
                newParticle.position = position;
                newParticle.velocity = Scale(normDir, e.velocity);
                particles.push_back(newParticle);
            }
        }
    }

    void rotate(const double &angleInRadians)
    {
        for (size_t eIdx = 0; eIdx < _emitters.size(); eIdx++)
        {
            ParticleEmitterObject &e = _emitters[eIdx];
            e.direction = Rotate(e.direction, static_cast<float>(angleInRadians));
        }
    }

public:
    void update(const double &simulationDeltaTime, std::vector<ParticleObject> &particles, const double &angleInRadians)
    {
        spawn(simulationDeltaTime, particles);
        rotate(angleInRadians);
    }

    void render(SDL_Renderer *r)
    {
        const float ppu = Globals::pixelsPerMeter;
        for (size_t eIdx = 0; eIdx < _emitters.size(); eIdx++)
        {
            ParticleEmitterObject &e = _emitters[eIdx];

            if (e.amount <= 0 || e.size < 0)
                continue;

            const float dirLen = Length(e.direction);
            if (dirLen < 1e-8f)
                continue;

            SDL_FPoint dir = {e.direction.x / dirLen, e.direction.y / dirLen};

            SDL_FPoint endPos = Add(e.position, Scale(dir, 30.0f / ppu));

            Colors::Orange(r);
            SDL_RenderLine(r, e.position.x * ppu, e.position.y * ppu, endPos.x * ppu, endPos.y * ppu);

            SDL_FPoint normal = GetNormal(dir);
            const float normalLen = Length(normal);
            if (normalLen < 1e-8f)
                continue;
            normal = {normal.x / normalLen, normal.y / normalLen};

            SDL_FPoint plane = Scale(normal, -e.size);
            SDL_FPoint planeStart = Add(e.position, plane);
            SDL_FPoint planeEnd = Sub(e.position, plane);

            SDL_RenderLine(r, planeStart.x * ppu, planeStart.y * ppu, planeEnd.x * ppu, planeEnd.y * ppu);

            for (int j = 0; j < static_cast<int>(e.amount); j++)
            {
                float t = static_cast<float>(j) / (e.amount - 1.0f > 0 ? e.amount - 1.0f : 1.0f);
                SDL_FPoint circlePos = Add(planeStart, Scale(Sub(planeEnd, planeStart), t));
                circlePos = {circlePos.x * ppu, circlePos.y * ppu};
                Shape::shape(r, circlePos, 5, Globals::PARTICLE_RESOLUTION, Colors::OrangeF());
            }
        }
    }
};

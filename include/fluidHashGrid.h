#pragma once
#include <unordered_map>
#include <vector>
#include <SDL3/SDL.h>
#include "particleobject.h"

class FluidHashGrid
{
public:
    double cellSize;
    std::unordered_map<long long, std::vector<size_t> > hashMap;
    size_t hashMapSize = 100000000;
    long long prime1 = 6614058611LL;
    long long prime2 = 7528850467LL;

    const std::vector<ParticleObject> *particles = nullptr;

    FluidHashGrid(float cellSize) : cellSize(cellSize) {}

    void initialize(std::vector<ParticleObject> &source)
    {
        particles = &source;
    }

    void clearGrid()
    {
        hashMap.clear();
    }

    uint32_t cellIndexToHash(const int cellX, const int cellY)
    {
        const uint32_t hash = (uint32_t)(((long long)cellX * prime1 ^ (long long)cellY * prime2) % (long long)hashMapSize);
        return hash;
    }

    SDL_FPoint getGridIndexFromPos(const SDL_FPoint &pos)
    {
        const float cellX = (float)(int)(pos.x / cellSize);
        const float cellY = (float)(int)(pos.y / cellSize);

        return {cellX, cellY};
    }

    long long getGridHashFromPos(const SDL_FPoint &pos)
    {
        SDL_FPoint cellIndex = getGridIndexFromPos(pos);

        return cellIndexToHash((int)cellIndex.x, (int)cellIndex.y);
    }

    std::vector<size_t> getNeighbourOfParticleIdx(size_t i)
    {
        std::vector<size_t> neighbours;
        if (!particles) return neighbours;
        SDL_FPoint pos = particles->at(i).position;

        SDL_FPoint cellIndex = getGridIndexFromPos(pos);

        for (int x = -1; x <= 1; x++)
        {
            for (int y = -1; y <= 1; y++)
            {
                int gridX = (int)cellIndex.x + x;
                int gridY = (int)cellIndex.y + y;

                long long hashId = cellIndexToHash(gridX, gridY);
                const auto &content = getContentOffCell(hashId);

                neighbours.insert(neighbours.end(), content.begin(), content.end());
            }
        }

        return neighbours;
    }

    void mapParticleToCell()
    {
        if (!particles) return;
        for (size_t i = 0; i < particles->size(); i++)
        {
            long long hash = getGridHashFromPos(particles->at(i).position);

            hashMap[hash].push_back(i);
        }
    }

    std::vector<size_t> &getContentOffCell(long long id)
    {
        auto it = hashMap.find(id);
        if (it != hashMap.end())
        {
            return it->second;
        }

        static std::vector<size_t> emptyVector;
        return emptyVector;
    }
};

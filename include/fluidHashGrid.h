#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include "particleobject.h"
#include "Perf.h"

class FluidHashGrid
{
public:
    double cellSize;
    static constexpr size_t TABLE_SIZE = 65536;
    std::vector<size_t> *buckets;
    int *bucketFrame;

    int currentFrame = 0;

    const std::vector<ParticleObject> *particles = nullptr;

    FluidHashGrid(float cellSize) : cellSize(cellSize)
    {
        buckets = new std::vector<size_t>[TABLE_SIZE];
        bucketFrame = new int[TABLE_SIZE];
        for (size_t i = 0; i < TABLE_SIZE; i++)
            bucketFrame[i] = -1;
    }

    ~FluidHashGrid()
    {
        delete[] buckets;
        delete[] bucketFrame;
    }

    FluidHashGrid(const FluidHashGrid &) = delete;
    FluidHashGrid &operator=(const FluidHashGrid &) = delete;

    void initialize(std::vector<ParticleObject> &source)
    {
        particles = &source;
    }

    void clearGrid()
    {
        PERF_SCOPE("Grid::clear");
    }

    uint32_t cellIndexToHash(const int cellX, const int cellY)
    {
        uint64_t hx = (uint64_t)cellX * 73856093ULL;
        uint64_t hy = (uint64_t)cellY * 19349663ULL;
        uint64_t h = hx ^ hy;
        return (uint32_t)(h & (TABLE_SIZE - 1));
    }

    SDL_FPoint getGridIndexFromPos(const SDL_FPoint &pos)
    {
        const float cellX = (float)(int)(pos.x / cellSize);
        const float cellY = (float)(int)(pos.y / cellSize);
        return {cellX, cellY};
    }

    uint32_t getGridHashFromPos(const SDL_FPoint &pos)
    {
        SDL_FPoint cellIndex = getGridIndexFromPos(pos);
        return cellIndexToHash((int)cellIndex.x, (int)cellIndex.y);
    }

    void mapParticleToCell()
    {
        PERF_SCOPE("Grid::mapParticleToCell");
        if (!particles)
            return;
        currentFrame++;
        const size_t count = particles->size();
        for (size_t i = 0; i < count; i++)
        {
            uint32_t hash = getGridHashFromPos(particles->at(i).position);
            if (bucketFrame[hash] != currentFrame)
            {
                bucketFrame[hash] = currentFrame;
                buckets[hash].clear();
            }
            buckets[hash].push_back(i);
        }
    }
};

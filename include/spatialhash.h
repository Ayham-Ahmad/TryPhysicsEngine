#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <utility>
#include <vector>

#include "parallel.h"

// Uniform-grid spatial hash that turns the O(n^2) neighbour search of the SPH
// density / pressure sweeps into roughly O(n * avgNeighbours).
// Port of Sebastian Lague's "Coding Adventure: Simulating Fluids" implementation.
class SpatialHash
{
public:
    // Builds (or rebuilds) the lookup from points, using radius as the cell size.
    // The points vector must stay alive and unchanged while queries are made.
    void update(const std::vector<SDL_FPoint> &points, float radius);

    // Calls callback(particleIndex, sqrDistanceToSample) for every stored point inside
    // radius of samplePoint. Must be called with the same radius update() used (the
    // spatial lookup is rebuilt every simulation step anyway).
    template <typename Fn>
    void forEachPointWithinRadius(const SDL_FPoint &samplePoint, Fn &&callback) const
    {
        const auto [centreX, centreY] = positionToCellCoord(samplePoint, _radius);
        const float sqrRadius = _radius * _radius;

        // 3x3 block around the centre cell is enough: a point within radius of the
        // sample can never land in a cell further than one cell away.
        for (int offsetY = -1; offsetY <= 1; offsetY++)
        {
            for (int offsetX = -1; offsetX <= 1; offsetX++)
            {
                const uint32_t key = getKeyFromHash(hashCell(centreX + offsetX, centreY + offsetY));

                for (size_t i = _startIndices[key]; i < _lookup.size(); i++)
                {
                    if (_lookup[i].cellKey != key)
                        break;

                    const int particleIndex = _lookup[i].particleIndex;

                    const SDL_FPoint &point = (*_points)[particleIndex];

                    const float dx = point.x - samplePoint.x;
                    const float dy = point.y - samplePoint.y;
                    const float sqrDst = dx * dx + dy * dy;

                    if (sqrDst <= sqrRadius)
                        callback(static_cast<size_t>(particleIndex), sqrDst);
                }
            }
        }
    }

private:
    struct Entry
    {
        int particleIndex;
        uint32_t cellKey;
    };

    static std::pair<int, int> positionToCellCoord(const SDL_FPoint &point, float radius);
    static uint32_t hashCell(int cellX, int cellY);
    uint32_t getKeyFromHash(uint32_t hash) const;

    const std::vector<SDL_FPoint> *_points = nullptr;
    float _radius = 1.0f;

    std::vector<Entry> _lookup;
    // First index in _lookup for every possible cell key (key < lookup size).
    // startIndices[key] == UINT32_MAX when a cell holds no points.
    std::vector<uint32_t> _startIndices;
};
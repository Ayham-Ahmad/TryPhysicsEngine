#include "spatialhash.h"
#include "Globals.h"

#include <algorithm>

std::pair<int, int> SpatialHash::positionToCellCoord(const SDL_FPoint &point, const float radius)
{
    const int cellX = static_cast<int>(point.x / radius);
    const int cellY = static_cast<int>(point.y / radius);

    return {cellX, cellY};
}

uint32_t SpatialHash::hashCell(const int cellX, const int cellY)
{
    const uint32_t a = static_cast<uint32_t>(cellX) * 15823u;
    const uint32_t b = static_cast<uint32_t>(cellY) * 9737333u;

    return a + b;
}

uint32_t SpatialHash::getKeyFromHash(const uint32_t hash) const
{
    // The lookup table is empty before the first update(); modulo by zero would
    // crash, so fall back to the raw hash until the table is built
    if (_lookup.empty())
        return hash;

    // Wrap the hash around the lookup length so it can be used as an index
    return hash % static_cast<uint32_t>(_lookup.size());
}

void SpatialHash::update(const std::vector<SDL_FPoint> &points, const float radius)
{
    _points = &points;
    _radius = radius;

    const size_t count = points.size();
    _lookup.resize(count);
    _startIndices.resize(count);

    // Build the (unordered) lookup and reset all start indices
    for (size_t i = 0; i < count; i++)
    {
        const auto [cellX, cellY] = positionToCellCoord(points[i], radius);
        _lookup[i] = {
            static_cast<int>(i),
            getKeyFromHash(hashCell(cellX, cellY))};
        _startIndices[i] = UINT32_MAX;
    }

    // Sort by cell key so the entries of one cell are contiguous
    std::sort(_lookup.begin(), _lookup.end(), [](const Entry &a, const Entry &b)
    {
        return a.cellKey < b.cellKey;
    });

    // The first entry of each unique cell key becomes that key's start index.
    // Every contiguous run of equal keys writes only its own unique slot.
    for (size_t i = 0; i < count; i++)
    {
        const uint32_t key = _lookup[i].cellKey;
        const uint32_t keyPrev = (i == 0) ? UINT32_MAX : _lookup[i - 1].cellKey;

        if (key != keyPrev)
            _startIndices[key] = static_cast<uint32_t>(i);
    }
}
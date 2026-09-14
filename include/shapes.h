#pragma once
#include <SDL3/SDL.h>
#include <unordered_map>
#include <vector>
#include <cmath>

#include "Globals.h"

// Precomputed polygon data, cached per resolution
struct ShapeData
{
    std::vector<SDL_FPoint> points;
    std::vector<int> indices;
};

class Shape
{
public:
    // Draw a shape
    static void shape(
        SDL_Renderer *r, const SDL_FPoint &position,
        const float radius, const int resolution,
        const SDL_FColor &color = {1.0f, 1.0f, 1.0f, 1.0f})
    {
        // Check if the resolution is valid
        if (resolution < 3)
            return;

        // Get the cached shape data
        const ShapeData &data = getShapeData(resolution);

        // Reusable vertex buffer (avoids allocating for every particle every frame)
        static thread_local std::vector<SDL_Vertex> vertices;
        vertices.resize(resolution + 1);

        // Set the center vertex
        vertices[0] = {position, color, {0, 0}};

        // Set the outer vertices
        for (int i = 0; i < resolution; ++i)
        {
            vertices[i + 1] = {
                {position.x + data.points[i].x * radius,
                 position.y + data.points[i].y * radius},
                color,
                {0, 0}};
        }

        // Draw the shape
        SDL_RenderGeometry(
            r, nullptr, vertices.data(),
            static_cast<int>(vertices.size()),
            data.indices.data(),
            static_cast<int>(data.indices.size()));
    }

    // Source - https://stackoverflow.com/a/48291620
    // Posted by Scotty Stephens, modified by community. See post 'Timeline' for change history
    // Retrieved 2026-09-08, License - CC BY-SA 4.0

    // Draw an empty circle
    static void emptyCircle(SDL_Renderer *r, int16_t cx, int16_t cy, int16_t radius)
    {
        // Check if the radius is valid
        if (radius <= 0)
            return;

        // Get the cached circle points
        const auto &points = getCircleOutlinePoints(radius);

        // Draw each point
        for (const SDL_FPoint &point : points)
            SDL_RenderPoint(r, cx + point.x, cy + point.y);
    }

    inline static SDL_Texture *circleTexture(SDL_Renderer *r)
    {
        const int size = Globals::smoothingRadius * 2;
        SDL_Surface *surface = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_RGBA32);

        SDL_FillSurfaceRect(surface, nullptr, SDL_MapSurfaceRGBA(surface, 0, 0, 0, 0));

        Uint32 *pixels = static_cast<Uint32 *>(surface->pixels);

        float radius = size / 2.0f;
        float center = radius;

        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                float dx = x - center;
                float dy = y - center;

                float distance = std::sqrt(dx * dx + dy * dy);

                if (distance <= radius)
                {
                    float alpha = 255.0f;

                    // Start fading near the edge
                    if (distance > radius - Globals::smoothingRadius)
                    {
                        float t = (radius - distance) / Globals::smoothingRadius;
                        alpha = t * 255.0f;
                    }

                    pixels[y * (surface->pitch / 4) + x] =
                        SDL_MapSurfaceRGBA(
                            surface,
                            255,
                            255,
                            255,
                            static_cast<Uint8>(alpha));
                }
            }
        }

        SDL_Texture *circleTexture = SDL_CreateTextureFromSurface(r, surface);
        SDL_DestroySurface(surface);

        return circleTexture;
    }

private:
    // Get cached shape data (points and indices)
    static const ShapeData &getShapeData(int resolution)
    {
        // Check if the data is already cached
        auto cached = _shapeDataCache.find(resolution);
        if (cached != _shapeDataCache.end())
            return cached->second;

        // Create the shape data
        ShapeData data;
        data.points.reserve(resolution);
        data.indices.reserve(resolution * 3);

        // Calculate each point around the shape
        for (int i = 0; i < resolution; ++i)
        {
            const float angle = 2.0f * Globals::PI * i / resolution;
            data.points.push_back({std::cos(angle), std::sin(angle)});
        }

        // Create the triangles
        for (int i = 0; i < resolution; ++i)
        {
            const int next = (i + 1) % resolution;

            data.indices.push_back(0);
            data.indices.push_back(i + 1);
            data.indices.push_back(next + 1);
        }

        // Store the data in the cache
        return _shapeDataCache.emplace(resolution, std::move(data)).first->second;
    }

    // Get cached circle outline points
    static const std::vector<SDL_FPoint> &getCircleOutlinePoints(int16_t radius)
    {
        // Check if the points are already cached
        auto cached = _circleOutlineCache.find(radius);
        if (cached != _circleOutlineCache.end())
            return cached->second;

        // Create the circle points
        std::vector<SDL_FPoint> points;
        const int16_t diameter = radius * 2;
        int16_t x = radius - 1;
        int16_t y = 0;
        int16_t tx = 1;
        int16_t ty = 1;
        int16_t error = tx - diameter;

        // Calculate the circle outline
        while (x >= y)
        {
            points.push_back({static_cast<float>(x), static_cast<float>(-y)});
            points.push_back({static_cast<float>(x), static_cast<float>(y)});
            points.push_back({static_cast<float>(-x), static_cast<float>(-y)});
            points.push_back({static_cast<float>(-x), static_cast<float>(y)});
            points.push_back({static_cast<float>(y), static_cast<float>(-x)});
            points.push_back({static_cast<float>(y), static_cast<float>(x)});
            points.push_back({static_cast<float>(-y), static_cast<float>(-x)});
            points.push_back({static_cast<float>(-y), static_cast<float>(x)});

            // Update the Y position
            if (error <= 0)
            {
                ++y;
                error += ty;
                ty += 2;
            }

            // Update the X position
            if (error > 0)
            {
                --x;
                tx += 2;
                error += tx - diameter;
            }
        }

        // Store the points in the cache
        return _circleOutlineCache.emplace(radius, std::move(points)).first->second;
    }

    // Store cached shape data
    inline static std::unordered_map<int, ShapeData> _shapeDataCache;

    // Store cached circle points
    inline static std::unordered_map<int, std::vector<SDL_FPoint>> _circleOutlineCache;
};
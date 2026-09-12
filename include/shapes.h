#pragma once
#include <SDL3/SDL.h>
#include <unordered_map>
#include <vector>
#include <cmath>

#include "Globals.h"

class Shape
{
public:
    // static void triangle(
    //     SDL_Renderer *r,
    //     const SDL_FPoint &a,
    //     const SDL_FPoint &b,
    //     const SDL_FPoint &c,
    //     const SDL_FColor &color)
    // {
    //     SDL_Vertex vertices[3] = {
    //         {b, color, {0, 0}},
    //         {a, color, {0, 0}},
    //         {c, color, {0, 0}}};

    //     SDL_RenderGeometry(r, nullptr, vertices, 3, nullptr, 0);
    // }

    // Draw a shape
    static void shape(
        SDL_Renderer *r, const SDL_FPoint &position,
        const float radius, const int8_t resolution,
        const SDL_FColor &color = {1.0f, 1.0f, 1.0f, 1.0f})
    {
        // Check if the resolution is valid
        if (resolution < 3)
            return;

        // Get the cached shape points
        const auto &points = getShapePoints(resolution);

        // Create the vertices and indices
        std::vector<SDL_Vertex> vertices(resolution + 1);
        std::vector<int> indices(resolution * 3);

        // Set the center vertex
        vertices[0] = {position, color, {0, 0}};

        // Set the outer vertices
        for (int i = 0; i < resolution; ++i)
        {
            vertices[i + 1] = {
                {
                    position.x + points[i].x * radius,
                    position.y + points[i].y * radius
                },
                color,
                {0, 0}
            };
        }

        // Create the triangles
        for (int i = 0; i < resolution; ++i) {
            const int next = (i + 1) % resolution;

            indices[i * 3 + 0] = 0;
            indices[i * 3 + 1] = i + 1;
            indices[i * 3 + 2] = next + 1;
        }

        // Draw the shape
        SDL_RenderGeometry(
            r, nullptr, vertices.data(),
            static_cast<int>(vertices.size()),
            indices.data(),
            static_cast<int>(indices.size())
        );
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

private:
    // Get cached shape points
    static const std::vector<SDL_FPoint> &getShapePoints(int resolution)
    {
        // Check if the points are already cached
        auto cached = shapeCache.find(resolution);
        if (cached != shapeCache.end())
            return cached->second;

        // Create the shape points
        std::vector<SDL_FPoint> points;
        points.reserve(resolution);

        // Calculate each point around the shape
        for (int i = 0; i < resolution; ++i)
        {
            const float angle = 2.0f * Globals::pi * i / resolution;
            points.push_back({std::cos(angle), std::sin(angle)});
        }

        // Store the points in the cache
        return shapeCache.emplace(resolution, std::move(points)).first->second;
    }

    // Get cached circle outline points
    static const std::vector<SDL_FPoint> &getCircleOutlinePoints(int16_t radius)
    {
        // Check if the points are already cached
        auto cached = circleOutlineCache.find(radius);
        if (cached != circleOutlineCache.end())
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
        return circleOutlineCache.emplace(radius, std::move(points)).first->second;
    }

    // Store cached shape points
    inline static std::unordered_map<int, std::vector<SDL_FPoint>> shapeCache;

    // Store cached circle points
    inline static std::unordered_map<int, std::vector<SDL_FPoint>> circleOutlineCache;
};

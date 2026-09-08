#pragma once
#include <SDL3/SDL.h>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <cmath>
#include <utility>
#include <stdio.h>

#include "Globals.h"
// #include "Colors.h"

class Shape
{
public:
    static void triangle(
        SDL_Renderer *r,
        const SDL_FPoint &a,
        const SDL_FPoint &b,
        const SDL_FPoint &c,
        const SDL_FColor &color)
    {
        SDL_Vertex v[3] = {
            {b, color, {0, 0}},
            {a, color, {0, 0}},
            {c, color, {0, 0}}};

        SDL_RenderGeometry(r, nullptr, v, 3, nullptr, 0);
    }

    static void circle(
        SDL_Renderer *r, const SDL_FPoint &p,
        const float rad, const int8_t res,
        const SDL_FColor &color = {1.0f, 1.0f, 1.0f, 1.0f})
    {
        if (res <= 0)
            return;

        const auto &points = getCirclePoints(res);

        for (int i = 0; i < res; ++i)
        {
            const int next = (i + 1) % res;
            const SDL_FPoint a = {
                p.x + points[i].x * rad,
                p.y + points[i].y * rad};
            const SDL_FPoint b = {
                p.x + points[next].x * rad,
                p.y + points[next].y * rad};

            triangle(r, p, a, b, color);
        }
    }

    // Source - https://stackoverflow.com/a/48291620
    // Posted by Scotty Stephens, modified by community. See post 'Timeline' for change history
    // Retrieved 2026-09-08, License - CC BY-SA 4.0

    static void circleE(SDL_Renderer *r, int16_t cx, int16_t cy, int16_t rad)
    {
        if (rad <= 0)
            return;

        const auto &points = getCircleOutlinePoints(rad);

        for (const SDL_FPoint &point : points)
            SDL_RenderPoint(r, cx + point.x, cy + point.y);
    }

private:
    static const std::vector<SDL_FPoint> &getCirclePoints(int res)
    {
        auto cached = circleCache.find(res);
        if (cached != circleCache.end())
            return cached->second;

        std::vector<SDL_FPoint> points;
        points.reserve(res);

        for (int i = 0; i < res; ++i)
        {
            const float angle = 2.0f * PI * i / res;
            points.push_back({std::cos(angle), std::sin(angle)});
        }

        return circleCache.emplace(res, std::move(points)).first->second;
    }

    static const std::vector<SDL_FPoint> &getCircleOutlinePoints(int16_t rad)
    {
        auto cached = circleOutlineCache.find(rad);
        if (cached != circleOutlineCache.end())
            return cached->second;

        std::vector<SDL_FPoint> points;
        const int16_t diameter = rad * 2;
        int16_t x = rad - 1;
        int16_t y = 0;
        int16_t tx = 1;
        int16_t ty = 1;
        int16_t error = tx - diameter;

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

            if (error <= 0)
            {
                ++y;
                error += ty;
                ty += 2;
            }

            if (error > 0)
            {
                --x;
                tx += 2;
                error += tx - diameter;
            }
        }

        return circleOutlineCache.emplace(rad, std::move(points)).first->second;
    }

    inline static std::unordered_map<int, std::vector<SDL_FPoint>> circleCache;
    inline static std::unordered_map<int, std::vector<SDL_FPoint>> circleOutlineCache;
};

// Printing
template <typename T>
void print(const T &value)
{
    std::cout << value << std::endl;
}

template <typename T, typename... Args>
void print(const T &first, const Args &...rest)
{
    std::cout << first << " ";
    print(rest...);
}

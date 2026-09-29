#pragma once
#include <SDL3/SDL.h>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iterator>

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

    // Draw an arrow (line + V-shaped head) from `from` to `to`.
    // If the two points coincide (zero-length, i.e. flat field) draw a small dot instead.
    static void arrow(SDL_Renderer *r, const SDL_FPoint &from, const SDL_FPoint &to,
                      const SDL_FColor &color = {1.0f, 1.0f, 0.0f, 1.0f})
    {
        SDL_SetRenderDrawColorFloat(r, color.r, color.g, color.b, color.a);

        const float dx = to.x - from.x;
        const float dy = to.y - from.y;
        const float length = std::sqrt(dx * dx + dy * dy);

        if (length < 2.0f)
        {
            const SDL_FRect dot = {from.x - 1.5f, from.y - 1.5f, 3.0f, 3.0f};
            SDL_RenderFillRect(r, &dot);
            return;
        }

        SDL_RenderLine(r, from.x, from.y, to.x, to.y);

        const float angle = std::atan2(dy, dx);
        const float headLen = std::max(4.0f, length * 0.3f);
        constexpr float spread = 0.42f;

        SDL_RenderLine(r, to.x, to.y,
                       to.x - headLen * std::cos(angle - spread),
                       to.y - headLen * std::sin(angle - spread));
        SDL_RenderLine(r, to.x, to.y,
                       to.x - headLen * std::cos(angle + spread),
                       to.y - headLen * std::sin(angle + spread));
    }

    // Soft glow field around a particle (premultiplied, additive so overlapping particles flow together)
    inline static SDL_Texture *circleTexture(SDL_Renderer *r, float radius)
    {
        constexpr float maxRadius = 100.0f;

        if (radius < 1.0f)
            radius = 1.0f;

        const int size = static_cast<int>(std::ceil(radius * 2.0f));
        SDL_Surface *surface = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_RGBA32);

        SDL_FillSurfaceRect(surface, nullptr, SDL_MapSurfaceRGBA(surface, 0, 0, 0, 0));

        Uint32 *pixels = static_cast<Uint32 *>(surface->pixels);

        const float center = radius;

        // Peak alpha: 255 when the field is tiny, never below 100 for very large fields
        const float peak = std::max(100.0f, 255.0f - (radius / maxRadius) * (255.0f - 100.0f));

        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                const float dx = x - center;
                const float dy = y - center;

                const float distance = std::sqrt(dx * dx + dy * dy);

                if (distance > radius)
                    continue;

                // Smoothstep falloff so overlapping fields merge without visible boundaries
                float t = 1.0f - distance / radius;
                t = t * t * (3.0f - 2.0f * t);

                const float alpha = peak * t;

                // Premultiplied color: the gradient rides in rgb so additive blending accumulates smoothly
                const float premult = alpha / 255.0f;

                pixels[y * (surface->pitch / 4) + x] =
                    SDL_MapSurfaceRGBA(
                        surface,
                        0,
                        static_cast<Uint8>(255.0f * premult),
                        static_cast<Uint8>(255.0f * premult),
                        static_cast<Uint8>(alpha));
            }
        }

        SDL_Texture *circleTexture = SDL_CreateTextureFromSurface(r, surface);
        SDL_DestroySurface(surface);

        SDL_SetTextureBlendMode(circleTexture, SDL_BLENDMODE_ADD);

        return circleTexture;
    }

    // Cached soft glow field texture (B5). Regenerates only when the smoothing radius
    // or particle size changes and owns its SDL_Texture, so nothing leaks and the
    // caller only ever holds one object.
    struct GlowCache
    {
        SDL_Texture *texture = nullptr;
        float smoothing = -1.0f;
        float particleSize = -1.0f;

        GlowCache() = default;
        GlowCache(const GlowCache &) = delete;
        GlowCache &operator=(const GlowCache &) = delete;

        ~GlowCache()
        {
            if (texture)
            {
                SDL_DestroyTexture(texture);
                texture = nullptr;
            }
        }

        // Get the cached texture, rebuilding it only when the smoothing radius or
        // particle size changes
        SDL_Texture *get(SDL_Renderer *r)
        {
            const float currentSmoothing = Globals::smoothingRadius;
            const float currentParticleSize = Globals::particleSize;

            if (texture && smoothing == currentSmoothing && particleSize == currentParticleSize)
                return texture;

            if (texture)
            {
                SDL_DestroyTexture(texture);
                texture = nullptr;
            }

            // The visual field is drawn in pixels, the sim is in meters
            const float outer = (Globals::particleSize + Globals::smoothingRadius) * Globals::pixelsPerMeter;

            texture = Shape::circleTexture(r, outer);
            smoothing = currentSmoothing;
            particleSize = currentParticleSize;

            // Linear filtering makes the additive glow merge smoothly (B5)
            SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);

            return texture;
        }
    };

    template <typename It, typename ColorFn>
    static void glowField(SDL_Renderer *r, It first, It last, ColorFn colorFn, GlowCache &glow)
    {
        SDL_Texture *tex = glow.get(r);

        // The visual field is drawn in pixels, the sim is in meters
        const float outer = (Globals::particleSize + Globals::smoothingRadius) * Globals::pixelsPerMeter;

        // Reusable scratch buffers (avoids allocating a new buffer every frame)
        static thread_local std::vector<SDL_Vertex> glowVertices;
        static thread_local std::vector<int> glowIndices;

        glowVertices.clear();
        glowIndices.clear();

        const size_t count = static_cast<size_t>(std::distance(first, last));

        glowVertices.reserve(count * 4);
        glowIndices.reserve(count * 6);

        size_t index = 0;
        for (; first != last; ++first, ++index)
        {
            const SDL_FPoint &pos = first->position;
            const SDL_FColor color = colorFn(index);

            const float x0 = pos.x * Globals::pixelsPerMeter - outer;
            const float y0 = pos.y * Globals::pixelsPerMeter - outer;
            const float x1 = x0 + outer * 2.0f;
            const float y1 = y0 + outer * 2.0f;

            const size_t base = glowVertices.size();

            glowVertices.push_back({{x0, y0}, color, {0.0f, 0.0f}});
            glowVertices.push_back({{x1, y0}, color, {1.0f, 0.0f}});
            glowVertices.push_back({{x1, y1}, color, {1.0f, 1.0f}});
            glowVertices.push_back({{x0, y1}, color, {0.0f, 1.0f}});

            glowIndices.push_back(static_cast<int>(base) + 0);
            glowIndices.push_back(static_cast<int>(base) + 1);
            glowIndices.push_back(static_cast<int>(base) + 2);
            glowIndices.push_back(static_cast<int>(base) + 0);
            glowIndices.push_back(static_cast<int>(base) + 2);
            glowIndices.push_back(static_cast<int>(base) + 3);
        }

        SDL_RenderGeometry(r, tex,
                           glowVertices.data(),
                           static_cast<int>(glowVertices.size()),
                           glowIndices.data(),
                           static_cast<int>(glowIndices.size()));
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
#pragma once
#include <SDL3/SDL.h>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iterator>

#include "Globals.h"

struct ShapeData
{
    std::vector<SDL_FPoint> points;
    std::vector<int> indices;
};

class Shape
{
public:
    static void shape(
        SDL_Renderer *r, const SDL_FPoint &position,
        const float radius, const int resolution,
        const SDL_FColor &color = {1.0f, 1.0f, 1.0f, 1.0f})
    {
        if (resolution < 3)
            return;

        const ShapeData &data = getShapeData(resolution);

        static thread_local std::vector<SDL_Vertex> vertices;
        vertices.resize(static_cast<size_t>(resolution));

        SDL_Vertex *v = vertices.data();
        const SDL_FPoint *pts = data.points.data();
        const float px = position.x;
        const float py = position.y;

        for (int i = 0; i < resolution; ++i)
        {
            v[i] = {
                {px + pts[i].x * radius, py + pts[i].y * radius},
                color,
                {0.0f, 0.0f}};
        }

        SDL_RenderGeometry(
            r, nullptr, v,
            resolution,
            data.indices.data(),
            static_cast<int>(data.indices.size()));
    }

    static void emptyCircle(SDL_Renderer *r, int16_t cx, int16_t cy, int16_t radius)
    {
        if (radius <= 0)
            return;

        const auto &points = getCircleOutlinePoints(radius);
        const size_t count = points.size();

        static thread_local std::vector<SDL_FPoint> translated;
        translated.resize(count);

        SDL_FPoint *dst = translated.data();
        const SDL_FPoint *src = points.data();
        const float fcx = static_cast<float>(cx);
        const float fcy = static_cast<float>(cy);

        for (size_t i = 0; i < count; ++i)
        {
            dst[i] = {fcx + src[i].x, fcy + src[i].y};
        }

        SDL_RenderPoints(r, dst, static_cast<int>(count));
    }

    static void arrow(SDL_Renderer *r, const SDL_FPoint &from, const SDL_FPoint &to,
                      const SDL_FColor &color = {1.0f, 1.0f, 0.0f, 1.0f})
    {
        SDL_SetRenderDrawColorFloat(r, color.r, color.g, color.b, color.a);

        const float dx = to.x - from.x;
        const float dy = to.y - from.y;
        const float lenSq = dx * dx + dy * dy;

        if (lenSq < 4.0f)
        {
            const SDL_FRect dot = {from.x - 1.5f, from.y - 1.5f, 3.0f, 3.0f};
            SDL_RenderFillRect(r, &dot);
            return;
        }

        SDL_RenderLine(r, from.x, from.y, to.x, to.y);

        const float length = std::sqrt(lenSq);
        const float invLen = 1.0f / length;
        const float headLen = std::max(4.0f, length * 0.3f);

        const float hx = dx * invLen * headLen;
        const float hy = dy * invLen * headLen;

        constexpr float cosSpread = 0.91308894f;
        constexpr float sinSpread = 0.40776045f;

        const SDL_FPoint head[3] = {
            {to.x - (hx * cosSpread + hy * sinSpread),
             to.y - (hy * cosSpread - hx * sinSpread)},
            {to.x, to.y},
            {to.x - (hx * cosSpread - hy * sinSpread),
             to.y - (hy * cosSpread + hx * sinSpread)}};

        SDL_RenderLines(r, head, 3);
    }

    inline static SDL_Texture *circleTexture(SDL_Renderer *r, float radius)
    {
        constexpr float maxRadius = 100.0f;

        if (radius < 1.0f)
            radius = 1.0f;

        const int size = static_cast<int>(std::ceil(radius * 2.0f));
        SDL_Surface *surface = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_RGBA32);

        const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(surface->format);
        SDL_FillSurfaceRect(surface, nullptr, SDL_MapRGBA(fmt, nullptr, 0, 0, 0, 0));

        Uint32 *pixels = static_cast<Uint32 *>(surface->pixels);
        const int stride = surface->pitch / 4;

        const float center = radius;
        const float radiusSq = radius * radius;
        const float invRadius = 1.0f / radius;
        const float peak = std::max(100.0f, 255.0f - (radius / maxRadius) * (255.0f - 100.0f));

        for (int y = 0; y < size; ++y)
        {
            const float dy = static_cast<float>(y) - center;
            const float dySq = dy * dy;
            Uint32 *row = pixels + y * stride;

            for (int x = 0; x < size; ++x)
            {
                const float dx = static_cast<float>(x) - center;
                const float distSq = dx * dx + dySq;

                if (distSq > radiusSq)
                    continue;

                const float distance = std::sqrt(distSq);
                float t = 1.0f - distance * invRadius;
                t = t * t * (3.0f - 2.0f * t);

                const float alpha = peak * t;
                const Uint8 a = static_cast<Uint8>(alpha);
                const Uint8 c = static_cast<Uint8>(alpha * (255.0f / 255.0f));

                row[x] = SDL_MapRGBA(fmt, nullptr, 0, c, c, a);
            }
        }

        SDL_Texture *circleTex = SDL_CreateTextureFromSurface(r, surface);
        SDL_DestroySurface(surface);

        SDL_SetTextureBlendMode(circleTex, SDL_BLENDMODE_ADD);

        return circleTex;
    }

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

            const float outer = (currentParticleSize + currentSmoothing) * Globals::pixelsPerMeter;

            texture = Shape::circleTexture(r, outer);
            smoothing = currentSmoothing;
            particleSize = currentParticleSize;

            SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);

            return texture;
        }
    };

    template <typename It, typename ColorFn>
    static void glowField(SDL_Renderer *r, It first, It last, ColorFn colorFn, GlowCache &glow)
    {
        if (first == last)
            return;

        SDL_Texture *tex = glow.get(r);

        const float ppm = Globals::pixelsPerMeter;
        const float outer = (Globals::particleSize + Globals::smoothingRadius) * ppm;

        static thread_local std::vector<SDL_Vertex> glowVertices;
        static thread_local std::vector<int> glowIndices;

        const size_t count = static_cast<size_t>(std::distance(first, last));
        const size_t requiredVertices = count * 4;
        const size_t requiredIndices = count * 6;

        glowVertices.resize(requiredVertices);

        const size_t currentQuads = glowIndices.size() / 6;
        if (count > currentQuads)
        {
            glowIndices.resize(requiredIndices);
            int *idx = glowIndices.data() + currentQuads * 6;
            for (size_t q = currentQuads; q < count; ++q)
            {
                const int base = static_cast<int>(q * 4);
                idx[0] = base + 0;
                idx[1] = base + 1;
                idx[2] = base + 2;
                idx[3] = base + 0;
                idx[4] = base + 2;
                idx[5] = base + 3;
                idx += 6;
            }
        }

        SDL_Vertex *v = glowVertices.data();
        size_t index = 0;

        for (; first != last; ++first, ++index)
        {
            const SDL_FPoint &pos = first->position;
            const SDL_FColor color = colorFn(index);

            const float cx = pos.x * ppm;
            const float cy = pos.y * ppm;
            const float x0 = cx - outer;
            const float y0 = cy - outer;
            const float x1 = cx + outer;
            const float y1 = cy + outer;

            v[0] = {{x0, y0}, color, {0.0f, 0.0f}};
            v[1] = {{x1, y0}, color, {1.0f, 0.0f}};
            v[2] = {{x1, y1}, color, {1.0f, 1.0f}};
            v[3] = {{x0, y1}, color, {0.0f, 1.0f}};
            v += 4;
        }

        SDL_RenderGeometry(
            r, tex,
            glowVertices.data(),
            static_cast<int>(requiredVertices),
            glowIndices.data(),
            static_cast<int>(requiredIndices));
    }

private:
    static ShapeData buildShapeData(int resolution)
    {
        ShapeData data;
        data.points.reserve(static_cast<size_t>(resolution));
        data.indices.reserve(static_cast<size_t>(resolution - 2) * 3);

        const float step = 2.0f * Globals::PI / static_cast<float>(resolution);
        for (int i = 0; i < resolution; ++i)
        {
            const float angle = step * static_cast<float>(i);
            data.points.push_back({std::cos(angle), std::sin(angle)});
        }

        for (int i = 1; i < resolution - 1; ++i)
        {
            data.indices.push_back(0);
            data.indices.push_back(i);
            data.indices.push_back(i + 1);
        }

        return data;
    }

    static const ShapeData &getShapeData(int resolution)
    {
        if (resolution < 128)
        {
            if (_fastShapeCache.size() <= static_cast<size_t>(resolution))
                _fastShapeCache.resize(128);

            ShapeData &entry = _fastShapeCache[static_cast<size_t>(resolution)];
            if (entry.points.empty())
                entry = buildShapeData(resolution);

            return entry;
        }

        auto cached = _shapeDataCache.find(resolution);
        if (cached != _shapeDataCache.end())
            return cached->second;

        return _shapeDataCache.emplace(resolution, buildShapeData(resolution)).first->second;
    }

    static const std::vector<SDL_FPoint> &getCircleOutlinePoints(int16_t radius)
    {
        auto cached = _circleOutlineCache.find(radius);
        if (cached != _circleOutlineCache.end())
            return cached->second;

        std::vector<SDL_FPoint> points;
        points.reserve(static_cast<size_t>(radius) * 6);

        const int16_t diameter = radius * 2;
        int16_t x = radius - 1;
        int16_t y = 0;
        int16_t tx = 1;
        int16_t ty = 1;
        int16_t error = tx - diameter;

        while (x >= y)
        {
            const float fx = static_cast<float>(x);
            const float fy = static_cast<float>(y);

            points.push_back({fx, -fy});
            points.push_back({fx, fy});
            points.push_back({-fx, -fy});
            points.push_back({-fx, fy});
            points.push_back({fy, -fx});
            points.push_back({fy, fx});
            points.push_back({-fy, -fx});
            points.push_back({-fy, fx});

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

        return _circleOutlineCache.emplace(radius, std::move(points)).first->second;
    }

    inline static std::vector<ShapeData> _fastShapeCache;
    inline static std::unordered_map<int, ShapeData> _shapeDataCache;
    inline static std::unordered_map<int, std::vector<SDL_FPoint>> _circleOutlineCache;
};
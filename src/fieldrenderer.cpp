#include "fieldrenderer.h"
#include "globals.h"
#include "particleobject.h"

#include <algorithm>
#include <cmath>

void FieldRenderer::update(const std::vector<ParticleObject> &particles, const float deltaTime) const
{
    _arrowRefreshTimer += deltaTime;
    if (_arrowRefreshTimer >= 1.0f / 30.0f)
    {
        _arrowRefreshTimer = 0.0f;
        refreshArrows(particles);
    }
}

void FieldRenderer::refreshArrows(const std::vector<ParticleObject> &particles) const
{
    _arrowCache.clear();

    // Sample grid in world (meter) units; ~0.2 m == 60 px at 300 px/m
    const float spacing = 0.2f;

    // Arrows start at the box center, so half the spacing keeps them inside their own grid box
    const float MAX_ARROW_LENGTH = spacing * 0.5f;

    const float ppu = Globals::pixelsPerMeter;

    struct Sample
    {
        SDL_FPoint pos;
        SDL_FPoint dir;
        float magnitude;
    };

    std::vector<Sample> samples;
    samples.reserve(static_cast<size_t>((Globals::worldW / spacing) * (Globals::worldH / spacing)));

    // Build a spatial lookup from the current particle positions (in meters) so the
    // per-sample density and force queries stay neighbour-only.
    std::vector<SDL_FPoint> positions(particles.size());
    for (size_t i = 0; i < particles.size(); i++)
        positions[i] = {particles[i].position.x, particles[i].position.y};

    SpatialHash hash;
    hash.update(positions, Globals::smoothingRadius);

    // Densities ρ_i only depend on particle positions (frozen during this refresh), so
    // compute all N once instead of N times per gradient call.
    std::vector<float> densities(particles.size());
    for (size_t i = 0; i < particles.size(); i++)
        densities[i] = _fluid.calculateDensity(positions[i], hash);

    for (float y = spacing * 0.5f; y < Globals::worldH; y += spacing)
    {
        for (float x = spacing * 0.5f; x < Globals::worldW; x += spacing)
        {
            const SDL_FPoint sample = {x, y};
            const float sampleDensity = _fluid.calculateDensity(sample, hash);
            const SDL_FPoint gradient = _fluid.calculatePressureForce(sample, sampleDensity, positions, densities, hash);

            const float magnitude = std::sqrt(gradient.x * gradient.x + gradient.y * gradient.y);
            if (magnitude < 1e-9f)
                continue;

            samples.push_back({sample,
                               {gradient.x / magnitude, gradient.y / magnitude},
                               magnitude});
        }
    }

    float maxMagnitude = 0.0f;
    for (const Sample &s : samples)
        maxMagnitude = std::max(maxMagnitude, s.magnitude);

    if (maxMagnitude < 1e-9f)
        return;

    for (const Sample &s : samples)
    {
        const float length = (s.magnitude / maxMagnitude) * MAX_ARROW_LENGTH;

        // Arrow geometry is sampled in meters, drawn in pixels
        if (length < 3.0f / ppu)
        {
            const SDL_FPoint posPx = {s.pos.x * ppu, s.pos.y * ppu};
            _arrowCache.emplace_back(GridArrow{posPx, posPx});
            continue;
        }

        const SDL_FPoint posPx = {s.pos.x * ppu, s.pos.y * ppu};
        const SDL_FPoint tipPx = {
            (s.pos.x + s.dir.x * length) * ppu,
            (s.pos.y + s.dir.y * length) * ppu};

        _arrowCache.emplace_back(GridArrow{posPx, tipPx});
    }
}

void FieldRenderer::renderArrows(SDL_Renderer *r) const
{
    // All arrows share the same colour and all segments are independent lines,
    // so emit the whole field as two batched submits (one for lines, one for dots).
    std::vector<SDL_FPoint> segments;
    std::vector<SDL_FRect> dots;
    segments.reserve(_arrowCache.size() * 3 * 2);

    for (const GridArrow &a : _arrowCache)
    {
        if (a.tip.x == a.pos.x && a.tip.y == a.pos.y)
        {
            dots.push_back({a.pos.x - 1.5f, a.pos.y - 1.5f, 3.0f, 3.0f});
            continue;
        }

        const float dx = a.tip.x - a.pos.x;
        const float dy = a.tip.y - a.pos.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        const float headLen = std::max(4.0f, length * 0.3f);
        constexpr float spread = 0.42f;
        const float angle = std::atan2(dy, dx);

        // Shaft
        segments.push_back(a.pos);
        segments.push_back(a.tip);

        // V-shaped head
        segments.push_back(a.tip);
        segments.push_back({a.tip.x - headLen * std::cos(angle - spread),
                            a.tip.y - headLen * std::sin(angle - spread)});
        segments.push_back(a.tip);
        segments.push_back({a.tip.x - headLen * std::cos(angle + spread),
                            a.tip.y - headLen * std::sin(angle + spread)});
    }

    SDL_SetRenderDrawColorFloat(r, 1.0f, 1.0f, 0.0f, 1.0f);

    if (!segments.empty())
        SDL_RenderLines(r, segments.data(), static_cast<int>(segments.size()));

    if (!dots.empty())
        SDL_RenderFillRects(r, dots.data(), static_cast<int>(dots.size()));
}
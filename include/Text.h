#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <unordered_map>

struct TextKey
{
    SDL_Renderer *r = nullptr;
    std::string text;
    int fontSize = 0;
    SDL_Color color = {255, 255, 255, 255};

    bool operator==(const TextKey &other) const
    {
        return r == other.r && text == other.text && fontSize == other.fontSize &&
               color.r == other.color.r && color.g == other.color.g &&
               color.b == other.color.b && color.a == other.color.a;
    }
};

struct TextKeyHash
{
    size_t operator()(const TextKey &key) const
    {
        size_t hash = std::hash<SDL_Renderer *>{}(key.r);
        hash = hash * 31 + std::hash<std::string>{}(key.text);
        hash = hash * 31 + std::hash<int>{}(key.fontSize);
        hash = hash * 31 + key.color.r + key.color.g * 256 + key.color.b * 65536;
        return hash;
    }
};

struct CachedText
{
    SDL_Texture *texture = nullptr;
    int width = 0;
    int height = 0;
    unsigned long long lastUsed = 0;
};

inline std::unordered_map<TextKey, CachedText, TextKeyHash> &textCache()
{
    static std::unordered_map<TextKey, CachedText, TextKeyHash> cache;
    return cache;
}

inline std::unordered_map<int, TTF_Font *> &fontCache()
{
    static std::unordered_map<int, TTF_Font *> fonts;
    return fonts;
}

// Remove all cached textures of a renderer (call before destroying it)
inline void purgeRendererTextures(SDL_Renderer *r)
{
    auto &cache = textCache();

    for (auto it = cache.begin(); it != cache.end();)
    {
        if (it->first.r == r)
        {
            SDL_DestroyTexture(it->second.texture);
            it = cache.erase(it);
        }
        else
            ++it;
    }
}

inline TTF_Font *cachedFont(const int fontSize)
{
    auto cached = fontCache().find(fontSize);
    if (cached != fontCache().end())
        return cached->second;

    TTF_Font *font = TTF_OpenFont("assets/Roboto-Medium.ttf", fontSize);
    if (!font)
        return nullptr;

    fontCache()[fontSize] = font;
    return font;
}

// Draw text using a string (cached for performance)
inline void drawText(SDL_Renderer *r,
                     const std::string &text,
                     float x = 100.0f, float y = 100.0f,
                     int fontSize = 10,
                     SDL_Color color = {255, 255, 255, 255},
                     float w = 0.0f, float h = 0.0f)
{
    if (!r || text.empty())
        return;

    static constexpr unsigned long long CACHE_LIMIT = 256;
    static unsigned long long drawClock = 0;
    drawClock++;

    TextKey key{r, text, fontSize, color};
    auto &cache = textCache();

    // Reuse a cached texture if available
    auto cached = cache.find(key);
    if (cached != cache.end())
    {
        cached->second.lastUsed = drawClock;

        SDL_FRect dst;
        dst.x = x;
        dst.y = y;
        dst.w = (w == 0.0f) ? static_cast<float>(cached->second.width) : w;
        dst.h = (h == 0.0f) ? static_cast<float>(cached->second.height) : h;

        SDL_RenderTexture(r, cached->second.texture, nullptr, &dst);
        return;
    }

    // Load the font (cached)
    TTF_Font *font = cachedFont(fontSize);
    if (!font)
        return;

    // Create a surface from the text
    SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), text.length(), color);
    if (!surface)
        return;

    // Create a texture from the surface
    SDL_Texture *texture = SDL_CreateTextureFromSurface(r, surface);

    int width = surface->w;
    int height = surface->h;
    SDL_DestroySurface(surface);

    if (!texture)
        return;

    // Remove the oldest cached text if the cache is full
    if (cache.size() >= CACHE_LIMIT)
    {
        auto oldest = cache.begin();

        for (auto it = cache.begin(); it != cache.end(); ++it)
        {
            if (it->second.lastUsed < oldest->second.lastUsed)
                oldest = it;
        }

        SDL_DestroyTexture(oldest->second.texture);
        cache.erase(oldest);
    }

    cache.emplace(key, CachedText{texture, width, height, drawClock});

    // Set the text position and size
    SDL_FRect dst;
    dst.x = x;
    dst.y = y;
    dst.w = (w == 0.0f) ? static_cast<float>(width) : w;
    dst.h = (h == 0.0f) ? static_cast<float>(height) : h;

    // Draw the text
    SDL_RenderTexture(r, texture, nullptr, &dst);
}

// Draw text using a float
inline void drawText(SDL_Renderer *r,
                     float value,
                     float x = 100.0f, float y = 100.0f,
                     int fontSize = 10,
                     float w = 0.0f, float h = 0.0f,
                     SDL_Color color = {255, 255, 255, 255})
{
    drawText(r, std::to_string(value), x, y, fontSize, color, w, h);
}

// Draw text using an int
inline void drawText(SDL_Renderer *r,
                     int value,
                     float x = 100.0f, float y = 100.0f,
                     int fontSize = 10,
                     float w = 0.0f, float h = 0.0f,
                     SDL_Color color = {255, 255, 255, 255})
{
    drawText(r, std::to_string(value), x, y, fontSize, color, w, h);
}
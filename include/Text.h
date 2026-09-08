#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

// --- Draw text using a string ---
inline void drawText(SDL_Renderer *r,
                     const std::string &text,
                     float x = 100.0f, float y = 100.0f,
                     int fontSize = 10,
                     float w = 0.0f, float h = 0.0f,
                     SDL_Color color = {255, 255, 255, 255},
                     const std::string &fontPath = "assets/Roboto-Medium.ttf")
{
    if (!r || text.empty())
        return;

    TTF_Font *font = TTF_OpenFont(fontPath.c_str(), fontSize);
    if (!font)
        return;

    SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), text.length(), color);
    if (!surface)
    {
        TTF_CloseFont(font);
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(r, surface);
    if (!texture)
    {
        SDL_DestroySurface(surface);
        TTF_CloseFont(font);
        return;
    }

    SDL_FRect dst;
    dst.x = x;
    dst.y = y;
    dst.w = (w == 0.0f) ? static_cast<float>(surface->w) : w;
    dst.h = (h == 0.0f) ? static_cast<float>(surface->h) : h;

    SDL_DestroySurface(surface);
    SDL_RenderTexture(r, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
    TTF_CloseFont(font);
}

// --- Draw text using a float ---
inline void drawText(SDL_Renderer *r,
                     float value,
                     float x = 100.0f, float y = 100.0f,
                     int fontSize = 10,
                     float w = 0.0f, float h = 0.0f,
                     SDL_Color color = {255, 255, 255, 255},
                     const std::string &fontPath = "assets/Roboto-Medium.ttf")
{
    drawText(r, std::to_string(value), x, y, fontSize, w, h, color, fontPath);
}

// --- Draw text using an int ---
inline void drawText(SDL_Renderer *r,
                     int value,
                     float x = 100.0f, float y = 100.0f,
                     int fontSize = 10,
                     float w = 0.0f, float h = 0.0f,
                     SDL_Color color = {255, 255, 255, 255},
                     const std::string &fontPath = "assets/Roboto-Medium.ttf")
{
    drawText(r, std::to_string(value), x, y, fontSize, w, h, color, fontPath);
}

inline void showXAndYPosition(SDL_Renderer *r, float &x, float &y, int xAdjustment = -10, int yAdjustment = -20)
{
    drawText(
        r,
        "(" + std::to_string(static_cast<int>(x)) + ", " +
            std::to_string(static_cast<int>(y)) + ")",
        x + xAdjustment, y + yAdjustment, 10);
}

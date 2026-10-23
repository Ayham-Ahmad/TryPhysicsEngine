#pragma once
#include "cmath"

#include "sdl3/SDL.h"

inline float Length(const SDL_FPoint &p)
{
    return std::sqrt(p.x * p.x + p.y * p.y);
}

inline float Length2(const SDL_FPoint &p)
{
    return p.x * p.x + p.y * p.y;
}

inline SDL_FPoint Noralize(const SDL_FPoint &p)
{
    float length = Length(p);
    return {
        p.x / length,
        p.y / length,
    };
}

inline SDL_FPoint GetNormal(const SDL_FPoint &p)
{
    return {
        p.y,
        -p.x,
    };
}

inline float Dot(const SDL_FPoint &firstP, const SDL_FPoint &secondP)
{
    return firstP.x * secondP.x + firstP.y * secondP.y;
}

static SDL_FPoint Zero()
{
    return {
        0, 0};
}

inline SDL_FPoint Add(const SDL_FPoint &firstP, const SDL_FPoint &secondP)
{
    return {
        firstP.x + secondP.x,
        firstP.y + secondP.y};
}

inline SDL_FPoint Sub(const SDL_FPoint &firstP, const SDL_FPoint &secondP)
{
    return {
        firstP.x - secondP.x,
        firstP.y - secondP.y};
}

inline SDL_FPoint Scale(const SDL_FPoint &firstP, const float &scalar)
{
    return {
        firstP.x * scalar,
        firstP.y * scalar};
}

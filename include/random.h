#pragma once

#include <random>

inline std::mt19937 &generator()
{
    static std::mt19937 engine{std::random_device{}()};
    return engine;
}

inline int randomInt(int min, int max)
{
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator());
}

inline float randomFloat(float min, float max)
{
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(generator());
}

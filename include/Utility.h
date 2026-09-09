#pragma once
#include <iostream>

// Print one value
template <typename T>
void print(const T &value)
{
    std::cout << value << std::endl;
}

// Print multiple values
template <typename T, typename... Args>
void print(const T &first, const Args &...rest)
{
    std::cout << first << " ";
    print(rest...);
}

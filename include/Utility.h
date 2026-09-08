#pragma once
#include <iostream>

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

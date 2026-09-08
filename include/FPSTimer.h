// code link: https://stackoverflow.com/questions/77744136/achieving-stable-60fps-with-sdl2

#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <type_traits>

template <std::intmax_t FPS>
class Timer
{
public:
    // initialize Timer with the current time point:
    Timer() : tp{std::chrono::steady_clock::now()} {}

    // a fixed duration with a length of 1/FPS seconds
    static constexpr std::chrono::duration<double, std::ratio<1, FPS>>
        time_between_frames{1};

    void sleep()
    {
        // add to the stored time point
        tp += time_between_frames;

        // and sleep almost until the new time point
        std::this_thread::sleep_until(tp - std::chrono::microseconds(100));

        // less than 100 microseconds busy wait
        while (std::chrono::steady_clock::now() < tp)
        {
        }
    }

private:
    // the time point we'll add to in every loop
    std::chrono::time_point<std::chrono::steady_clock,
                            std::remove_const_t<decltype(time_between_frames)>>
        tp;
};
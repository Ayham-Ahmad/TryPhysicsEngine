#pragma once

#include <algorithm>
#include <thread>
#include <vector>

// Splits the work into chunks and runs each chunk on its own thread.
//
// Usage:
//     parallelFor(array.size(), [&](size_t i)
//     {
//         // do something with array[i]
//     });

template <typename Fn>
void parallelFor(const size_t end, Fn &&body)
{
    parallelFor(size_t{0}, end, std::forward<Fn>(body));
}

template <typename Fn>
void parallelFor(const size_t begin, const size_t end, Fn &&body)
{
    if (begin >= end)
        return;

    const size_t total = end - begin;

    // Number of CPU cores available
    unsigned threads = std::thread::hardware_concurrency();
    if (threads == 0)
        threads = 1;

    // Don't create more threads than there is work
    if (threads > total)
        threads = static_cast<unsigned>(total);

    // Size of each chunk
    const size_t chunk = (total + threads - 1) / threads;

    // Start one worker thread per chunk (chunk 0 runs on this thread)
    std::vector<std::thread> workers;
    workers.reserve(threads - 1);

    for (unsigned t = 1; t < threads; t++)
    {
        const size_t chunkBegin = begin + t * chunk;
        const size_t chunkEnd = std::min(chunkBegin + chunk, end);

        if (chunkBegin >= chunkEnd)
            break;

        workers.emplace_back([&, chunkBegin, chunkEnd]
        {
            for (size_t i = chunkBegin; i < chunkEnd; i++)
                body(i);
        });
    }

    // This thread handles the first chunk
    const size_t firstEnd = std::min(begin + chunk, end);

    for (size_t i = begin; i < firstEnd; i++)
        body(i);

    // Wait for all worker threads to finish
    for (std::thread &worker : workers)
        worker.join();
}

// Same as parallelFor, but body() receives (chunkBegin, chunkEnd, threadId).
// Use this when each thread needs its OWN private accumulator variables,
// so multiple threads never add into the same variable (a "reduction"):
//
//     std::vector<float> partial(threads, 0.0f);
//     parallelChunks(count, [&](size_t begin, size_t end, unsigned t)
//     {
//         for (size_t i = begin; i < end; i++)
//             partial[t] += value[i];
//     });
//     // then sum partial[0] + partial[1] + ... on one thread
template <typename Fn>
void parallelChunks(const size_t total, Fn &&body)
{
    if (total == 0)
        return;

    // Number of CPU cores available
    unsigned threads = std::thread::hardware_concurrency();
    if (threads == 0)
        threads = 1;

    // Don't create more threads than there is work
    if (threads > total)
        threads = static_cast<unsigned>(total);

    // Size of each chunk
    const size_t chunk = (total + threads - 1) / threads;

    // Start one worker thread per chunk (chunk 0 runs on this thread)
    std::vector<std::thread> workers;
    workers.reserve(threads - 1);

    for (unsigned t = 1; t < threads; t++)
    {
        const size_t chunkBegin = t * chunk;
        const size_t chunkEnd = std::min(chunkBegin + chunk, total);

        if (chunkBegin >= chunkEnd)
            break;

        workers.emplace_back([&, chunkBegin, chunkEnd, t]
        {
            body(chunkBegin, chunkEnd, t);
        });
    }

    // This thread handles the first chunk
    body(0, std::min(chunk, total), 0);

    // Wait for all worker threads to finish
    for (std::thread &worker : workers)
        worker.join();
}
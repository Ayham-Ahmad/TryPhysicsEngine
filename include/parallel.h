#pragma once
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <algorithm>
#include <cstdint>

class Parallel
{
public:
    template <typename IndexA, typename IndexB, typename Func>
    static void forLoop(IndexA startIndex, IndexB endIndex, Func &&func)
    {
        const int start = static_cast<int>(startIndex);
        const int end = static_cast<int>(endIndex);
        const int total = end - start;

        if (total <= 0)
            return;

        ThreadPool &pool = getPool();
        const int numThreads = pool.threadCount();

        if (numThreads <= 1 || total < 64)
        {
            for (int i = start; i < end; ++i)
            {
                func(i);
            }
            return;
        }

        const int activeThreads = std::min(numThreads, total);
        const int chunkSize = total / activeThreads;
        const int remainder = total % activeThreads;

        pool.dispatch(activeThreads, [&](int threadId)
        {
            const int chunkStart = start + threadId * chunkSize + std::min(threadId, remainder);
            const int chunkEnd = chunkStart + chunkSize + (threadId < remainder ? 1 : 0);

            for (int i = chunkStart; i < chunkEnd; ++i)
            {
                func(i);
            }
        });
    }

    template <typename Index, typename Func>
    static void forLoop(Index count, Func &&func)
    {
        forLoop(0, static_cast<int>(count), std::forward<Func>(func));
    }

private:
    class ThreadPool
    {
    public:
        ThreadPool()
        {
            unsigned int hw = std::thread::hardware_concurrency();
            _numThreads = (hw > 0) ? static_cast<int>(hw) : 4;

            _workers.reserve(static_cast<size_t>(_numThreads));
            for (int t = 0; t < _numThreads; ++t)
            {
                _workers.emplace_back([this, t]()
                {
                    workerLoop(t);
                });
            }
        }

        ~ThreadPool()
        {
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _stop = true;
            }
            _cvWork.notify_all();

            for (std::thread &worker : _workers)
            {
                if (worker.joinable())
                    worker.join();
            }
        }

        int threadCount() const
        {
            return _numThreads;
        }

        template <typename TaskFn>
        void dispatch(int activeThreads, TaskFn &&task)
        {
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _activeThreads = activeThreads;
                _remaining = activeThreads;
                _currentTask = std::forward<TaskFn>(task);
                ++_generation;
            }

            _cvWork.notify_all();

            std::unique_lock<std::mutex> lock(_mutex);
            _cvDone.wait(lock, [this]()
            {
                return _remaining == 0;
            });
        }

    private:
        void workerLoop(int threadId)
        {
            uint64_t lastGeneration = 0;

            while (true)
            {
                std::function<void(int)> task;
                bool shouldRun = false;

                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _cvWork.wait(lock, [this, lastGeneration]()
                    {
                        return _stop || _generation != lastGeneration;
                    });

                    if (_stop)
                        return;

                    lastGeneration = _generation;
                    if (threadId < _activeThreads)
                    {
                        task = _currentTask;
                        shouldRun = true;
                    }
                }

                if (shouldRun)
                {
                    task(threadId);

                    std::lock_guard<std::mutex> lock(_mutex);
                    if (--_remaining == 0)
                    {
                        _cvDone.notify_one();
                    }
                }
            }
        }

        int _numThreads = 1;
        int _activeThreads = 0;
        int _remaining = 0;
        uint64_t _generation = 0;
        bool _stop = false;

        std::vector<std::thread> _workers;
        std::mutex _mutex;
        std::condition_variable _cvWork;
        std::condition_variable _cvDone;
        std::function<void(int)> _currentTask;
    };

    static ThreadPool &getPool()
    {
        static ThreadPool pool;
        return pool;
    }
};

template <typename IndexA, typename IndexB, typename Func>
inline void parallel_for(IndexA start, IndexB end, Func &&func)
{
    Parallel::forLoop(start, end, std::forward<Func>(func));
}

template <typename Index, typename Func>
inline void parallel_for(Index count, Func &&func)
{
    Parallel::forLoop(0, count, std::forward<Func>(func));
}
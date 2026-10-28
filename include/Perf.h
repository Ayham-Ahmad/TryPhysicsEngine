#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <algorithm>

struct PerfEntry
{
    const char* name = nullptr;
    double totalMs = 0.0;
    double count = 0.0;
};

class Perf
{
public:
    static Perf& Get()
    {
        static Perf p;
        return p;
    }

    void BeginFrame()
    {
        m_frameStart = SDL_GetPerformanceCounter();
    }

    void Begin(const char* name)
    {
        m_activeName = name;
        m_activeStart = SDL_GetPerformanceCounter();
    }

    void End()
    {
        if (m_activeName == nullptr) return;
        Uint64 end = SDL_GetPerformanceCounter();
        double ms = (end - m_activeStart) * 1000.0 / (double)SDL_GetPerformanceFrequency();
        Add(m_activeName, ms);
        m_activeName = nullptr;
    }

    void EndFrame()
    {
        Uint64 end = SDL_GetPerformanceCounter();
        m_frameTotalMs = (end - m_frameStart) * 1000.0 / (double)SDL_GetPerformanceFrequency();
    }

    void Add(const char* name, double ms)
    {
        for (auto &e : m_entries)
        {
            if (e.name == name)
            {
                e.totalMs += ms;
                e.count += 1.0;
                return;
            }
        }
        m_entries.push_back({name, ms, 1.0});
    }

    void ReportAndClear()
    {
        if (m_entries.empty() && m_frameTotalMs <= 0.0) return;

        if (m_frameTotalMs <= 0.0)
        {
            double total = 0.0;
            for (const auto &e : m_entries) if (e.count > 0.0) total += e.totalMs;
            m_frameTotalMs = total;
        }

        SDL_Log("\n\n==================== PERFORMANCE REPORT ====================");
        SDL_Log("Total frame time accumulated: %.3f ms (avg %.1f FPS)", m_frameTotalMs, m_frameTotalMs > 0.0 ? 1000.0 / m_frameTotalMs : 0.0);
        SDL_Log("-----------------------------------------------------------");
        SDL_Log("%-32s | %12s | %12s | %10s", "System", "Total (ms)", "Avg (ms)", "% of frame");
        SDL_Log("-----------------------------------------------------------");

        double sortedTotal = 0.0;
        std::vector<PerfEntry> sorted = m_entries;
        std::sort(sorted.begin(), sorted.end(), [](const PerfEntry &a, const PerfEntry &b) {
            return a.totalMs > b.totalMs;
        });

        for (const auto &e : sorted)
        {
            if (e.count <= 0.0 || e.totalMs <= 0.0) continue;
            double avg = e.totalMs / e.count;
            double pct = m_frameTotalMs > 0.0 ? (e.totalMs / m_frameTotalMs) * 100.0 : 0.0;
            sortedTotal += e.totalMs;
            SDL_Log("%-32s | %12.3f | %12.3f | %9.1f%%", e.name, e.totalMs, avg, pct);
        }

        SDL_Log("-----------------------------------------------------------");
        double overhead = m_frameTotalMs - sortedTotal;
        if (overhead < 0.0) overhead = 0.0;
        SDL_Log("%-32s | %12.3f | %12.3f | %9.1f%%", "Unaccounted/Other", overhead, overhead, m_frameTotalMs > 0.0 ? (overhead / m_frameTotalMs) * 100.0 : 0.0);
        SDL_Log("%-32s | %12.3f | %12.3f | %9.1f%%", "TOTAL", m_frameTotalMs, m_frameTotalMs, 100.0);
        SDL_Log("===========================================================\n\n");

        for (auto &e : m_entries)
        {
            e.totalMs = 0.0;
            e.count = 0.0;
        }
        m_entries.clear();
        m_frameTotalMs = 0.0;
        m_activeName = nullptr;
    }

    double m_frameTotalMs = 0.0;

private:
    Uint64 m_frameStart = 0;
    Uint64 m_activeStart = 0;
    const char* m_activeName = nullptr;
    std::vector<PerfEntry> m_entries;
};

struct PerfScope
{
    const char* n;
    Uint64 s;
    PerfScope(const char* name) { n = name; s = SDL_GetPerformanceCounter(); }
    ~PerfScope()
    {
        Uint64 e = SDL_GetPerformanceCounter();
        double ms = (e - s) * 1000.0 / (double)SDL_GetPerformanceFrequency();
        Perf::Get().Add(n, ms);
    }
};

#define PERF_SCOPE(name) PerfScope _perf_scope_##__LINE__(name)
#define PERF_BEGIN(name) Perf::Get().Begin(name)
#define PERF_END() Perf::Get().End()

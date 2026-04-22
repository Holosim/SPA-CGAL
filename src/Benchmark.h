#pragma once
#include <chrono>
#include <string>
#include <iostream>
#include <iomanip>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

// Returns current peak working-set size in bytes (Windows only; 0 elsewhere).
inline size_t peakMemoryBytes() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc{};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return pmc.PeakWorkingSetSize;
#endif
    return 0;
}

// RAII wall-clock timer. Writes elapsed milliseconds to `out` on destruction.
struct ScopedTimer {
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point start;
    double& out;

    explicit ScopedTimer(double& ms)
        : start(Clock::now()), out(ms) {}

    ~ScopedTimer() {
        out = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    }
};

struct BenchmarkRow {
    int    step;
    double stepMs;
    double cumMs;
    size_t faces;
    size_t vertices;
    size_t peakBytes;
};

inline void printBenchmarkHeader() {
    std::cout << "\n"
              << std::left
              << std::setw(6)  << "Step"
              << std::setw(13) << "Step(ms)"
              << std::setw(15) << "Cumul(ms)"
              << std::setw(10) << "Faces"
              << std::setw(12) << "Vertices"
              << "PeakMem(MB)\n"
              << std::string(68, '-') << "\n";
}

inline void printBenchmarkRow(const BenchmarkRow& r) {
    std::cout << std::left
              << std::setw(6)  << r.step
              << std::setw(13) << std::fixed << std::setprecision(1) << r.stepMs
              << std::setw(15) << r.cumMs
              << std::setw(10) << r.faces
              << std::setw(12) << r.vertices
              << std::fixed << std::setprecision(1)
              << (r.peakBytes / 1024.0 / 1024.0) << "\n";
}

inline void printBenchmarkSummary(const std::vector<BenchmarkRow>& rows,
                                   const std::string& label) {
    if (rows.empty()) return;
    const auto& last = rows.back();
    std::cout << "\n--- " << label << " Summary ---\n"
              << "  Steps          : " << rows.size() << "\n"
              << "  Total time     : " << std::fixed << std::setprecision(1)
              << last.cumMs << " ms\n"
              << "  Final faces    : " << last.faces << "\n"
              << "  Final vertices : " << last.vertices << "\n"
              << "  Peak memory    : " << std::fixed << std::setprecision(1)
              << (last.peakBytes / 1024.0 / 1024.0) << " MB\n";

    if (rows.size() > 1) {
        double minStep = rows[0].stepMs, maxStep = rows[0].stepMs, sum = 0;
        for (auto& r : rows) {
            minStep = std::min(minStep, r.stepMs);
            maxStep = std::max(maxStep, r.stepMs);
            sum += r.stepMs;
        }
        std::cout << "  Avg step time  : " << std::fixed << std::setprecision(1)
                  << (sum / rows.size()) << " ms  "
                  << "[min=" << minStep << "  max=" << maxStep << "]\n";
    }
}

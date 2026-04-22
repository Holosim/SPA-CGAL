#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdio>

#include "Config.h"
#include "MeshUtils.h"
#include "Benchmark.h"

// Collects input .obj file paths according to config:
//   - input_folder  : all .obj files in directory, sorted by name
//   - input_pattern : printf-style pattern (e.g. "mesh_%03d.obj") with
//                     input_start / input_end range
static std::vector<std::string> collectBatchFiles(const Config& cfg) {
    std::vector<std::string> paths;

    if (!cfg.inputFolder.empty()) {
        fs::path dir(cfg.inputFolder);
        if (!fs::is_directory(dir)) {
            std::cerr << "  Error: input_folder '" << cfg.inputFolder
                      << "' is not a directory.\n";
            return paths;
        }
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".obj" || ext == ".off" || ext == ".ply" || ext == ".stl")
                paths.push_back(entry.path().string());
        }
        std::sort(paths.begin(), paths.end());
    } else {
        // printf-style pattern mode
        char buf[1024];
        for (int i = cfg.inputStart; i <= cfg.inputEnd; ++i) {
            std::snprintf(buf, sizeof(buf), cfg.inputPattern.c_str(), i);
            paths.emplace_back(buf);
        }
    }

    return paths;
}

// Approach A: load multiple meshes and iteratively union them into one.
// Returns per-step benchmark rows.
inline std::vector<BenchmarkRow> runBatchMerge(const Config& cfg) {
    std::vector<BenchmarkRow> rows;

    std::cout << "\n=== Batch Merge ===\n";

    auto paths = collectBatchFiles(cfg);
    if (paths.empty()) {
        std::cerr << "  Error: no input files found.\n";
        return rows;
    }
    std::cout << "  Found " << paths.size() << " mesh(es) to merge.\n";

    Mesh accumulator;
    std::cout << "\nLoading seed mesh: " << paths[0] << "\n";
    if (!loadMesh(paths[0], accumulator))
        return rows;

    printBenchmarkHeader();

    double cumMs = 0.0;
    BenchmarkRow seed{};
    seed.step      = 0;
    seed.stepMs    = 0.0;
    seed.cumMs     = 0.0;
    seed.faces     = accumulator.number_of_faces();
    seed.vertices  = accumulator.number_of_vertices();
    seed.peakBytes = peakMemoryBytes();
    printBenchmarkRow(seed);
    rows.push_back(seed);

    for (size_t i = 1; i < paths.size(); ++i) {
        std::cout << "  [" << i << "] Merging: " << paths[i] << "\n";

        Mesh next;
        if (!loadMesh(paths[i], next, false)) {
            std::cerr << "  Skipping.\n";
            continue;
        }

        double stepMs = 0.0;
        {
            ScopedTimer t(stepMs);
            if (!unionInto(accumulator, next)) {
                std::cerr << "  Error: union failed at step " << i
                          << ". Skipping.\n";
                continue;
            }
        }
        cumMs += stepMs;

        BenchmarkRow r{};
        r.step      = static_cast<int>(i);
        r.stepMs    = stepMs;
        r.cumMs     = cumMs;
        r.faces     = accumulator.number_of_faces();
        r.vertices  = accumulator.number_of_vertices();
        r.peakBytes = peakMemoryBytes();
        printBenchmarkRow(r);
        rows.push_back(r);
    }

    printBenchmarkSummary(rows, "Batch Merge");

    std::cout << "\nWriting result to '" << cfg.outputPath << "'...\n";
    if (!writeMesh(cfg.outputPath, accumulator))
        return rows;
    std::cout << "  Done.\n";

    return rows;
}

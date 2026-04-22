#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

#include "Config.h"
#include "MeshUtils.h"
#include "Benchmark.h"

// Parses one row of 16 comma-separated doubles into a CGAL Aff_transformation_3.
// CSV row is row-major 4x4:
//   m00 m01 m02 m03
//   m10 m11 m12 m13
//   m20 m21 m22 m23
//   m30 m31 m32 m33   <- ignored (always 0 0 0 1)
static bool parseTransformRow(const std::string& line, Xform& out) {
    std::istringstream ss(line);
    double m[16];
    char   comma;
    for (int i = 0; i < 16; ++i) {
        if (!(ss >> m[i])) return false;
        if (i < 15) ss >> comma; // optional separator
    }
    // CGAL Aff_transformation_3 takes the 3×4 upper block (row-major):
    //   (m00,m01,m02,m03, m10,m11,m12,m13, m20,m21,m22,m23)
    out = Xform(m[0],  m[1],  m[2],  m[3],
                m[4],  m[5],  m[6],  m[7],
                m[8],  m[9],  m[10], m[11]);
    return true;
}

static std::vector<Xform> loadTransformsCsv(const std::string& path) {
    std::vector<Xform> xforms;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "  Error: cannot open transforms CSV '" << path << "'\n";
        return xforms;
    }

    std::string line;
    int lineNo = 0;
    while (std::getline(file, line)) {
        ++lineNo;
        // Strip comments and whitespace
        auto hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;

        Xform xf;
        if (!parseTransformRow(line, xf)) {
            std::cerr << "  Warning: skipping malformed row at line "
                      << lineNo << "\n";
            continue;
        }
        xforms.push_back(xf);
    }
    return xforms;
}

// Approach B: load a single base mesh, apply each transform from a CSV,
// and union each transformed copy into a growing accumulator ("brush stroke").
// Returns per-step benchmark rows.
inline std::vector<BenchmarkRow> runTransformSweep(const Config& cfg) {
    std::vector<BenchmarkRow> rows;

    std::cout << "\n=== Transform Sweep ===\n";

    Mesh baseMesh;
    std::cout << "Loading base mesh: " << cfg.baseModel << "\n";
    if (!loadMesh(cfg.baseModel, baseMesh))
        return rows;
    std::cout << "  " << baseMesh.number_of_faces() << " faces, "
              << baseMesh.number_of_vertices() << " vertices\n";

    auto xforms = loadTransformsCsv(cfg.transformsCsv);
    if (xforms.empty()) {
        std::cerr << "  Error: no valid transforms in '" << cfg.transformsCsv << "'\n";
        return rows;
    }
    std::cout << "  Loaded " << xforms.size() << " transform(s).\n";

    // Seed the accumulator with the first transform applied to the base mesh.
    Mesh accumulator = baseMesh;
    applyTransform(accumulator, xforms[0]);

    printBenchmarkHeader();

    BenchmarkRow seed{};
    seed.step      = 0;
    seed.stepMs    = 0.0;
    seed.cumMs     = 0.0;
    seed.faces     = accumulator.number_of_faces();
    seed.vertices  = accumulator.number_of_vertices();
    seed.peakBytes = peakMemoryBytes();
    printBenchmarkRow(seed);
    rows.push_back(seed);

    double cumMs = 0.0;
    for (size_t i = 1; i < xforms.size(); ++i) {
        // Copy base mesh and apply next transform — base mesh is never mutated.
        Mesh brush = baseMesh;
        applyTransform(brush, xforms[i]);

        double stepMs = 0.0;
        {
            ScopedTimer t(stepMs);
            if (!unionInto(accumulator, brush)) {
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

    printBenchmarkSummary(rows, "Transform Sweep");

    std::cout << "\nWriting result to '" << cfg.outputPath << "'...\n";
    if (!writeMesh(cfg.outputPath, accumulator))
        return rows;
    std::cout << "  Done.\n";

    return rows;
}

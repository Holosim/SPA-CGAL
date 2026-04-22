#include <iostream>
#include <iomanip>
#include <string>

#include "Config.h"
#include "MeshUtils.h"
#include "Benchmark.h"
#include "BatchMerge.h"
#include "TransformSweep.h"

// ---------------------------------------------------------------------------
// BOOLEAN mode — original two-mesh boolean operation
// ---------------------------------------------------------------------------
static int runBoolean(const Config& cfg) {
    Mesh meshA, meshB;

    std::cout << "Loading primary model: " << cfg.primaryModel << "\n";
    if (!loadMesh(cfg.primaryModel, meshA)) return 1;
    std::cout << "  " << meshA.number_of_faces() << " faces, "
              << meshA.number_of_vertices() << " vertices\n\n";

    std::cout << "Loading secondary model: " << cfg.secondaryModel << "\n";
    if (!loadMesh(cfg.secondaryModel, meshB)) return 1;
    std::cout << "  " << meshB.number_of_faces() << " faces, "
              << meshB.number_of_vertices() << " vertices\n\n";

    std::cout << "Computing " << cfg.operation << "...\n";
    double ms = 0.0;
    Mesh result;
    bool ok = false;
    {
        ScopedTimer t(ms);
        if      (cfg.operation == "UNION")
            ok = PMP::corefine_and_compute_union(meshA, meshB, result);
        else if (cfg.operation == "INTERSECTION")
            ok = PMP::corefine_and_compute_intersection(meshA, meshB, result);
        else if (cfg.operation == "DIFFERENCE")
            ok = PMP::corefine_and_compute_difference(meshA, meshB, result);
    }

    if (!ok) {
        std::cerr << "  Error: boolean operation failed.\n";
        return 1;
    }
    std::cout << "  " << result.number_of_faces() << " faces, "
              << result.number_of_vertices() << " vertices  ["
              << std::fixed << std::setprecision(1) << ms << " ms]\n\n";

    std::cout << "Writing '" << cfg.outputPath << "'...\n";
    if (!writeMesh(cfg.outputPath, result)) return 1;
    std::cout << "  Done.\n";
    return 0;
}

// ---------------------------------------------------------------------------

int main() {
    std::cout << "SPA-CGAL Mesh Operations\n"
              << "=========================\n\n";

    std::string configPath;
    std::cout << "Config file path: ";
    std::getline(std::cin, configPath);

    if (configPath.size() >= 2 &&
        configPath.front() == '"' && configPath.back() == '"')
        configPath = configPath.substr(1, configPath.size() - 2);

    Config cfg;
    try {
        cfg = loadConfig(configPath);
    } catch (const std::exception& e) {
        std::cerr << "\nConfig error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Mode: " << cfg.mode << "\n";

    if      (cfg.mode == "BOOLEAN")         return runBoolean(cfg);
    else if (cfg.mode == "BATCH_MERGE")     { runBatchMerge(cfg);     return 0; }
    else if (cfg.mode == "TRANSFORM_SWEEP") { runTransformSweep(cfg); return 0; }

    std::cerr << "Unknown mode: " << cfg.mode << "\n";
    return 1;
}

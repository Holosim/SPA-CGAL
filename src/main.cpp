#include <iostream>
#include <string>
#include <filesystem>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/IO/polygon_mesh_io.h>

#include "Config.h"

namespace PMP = CGAL::Polygon_mesh_processing;
namespace fs  = std::filesystem;

using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using Mesh   = CGAL::Surface_mesh<Kernel::Point_3>;

// ----------------------------------------------------------------------------

static bool loadMesh(const std::string& path, Mesh& mesh) {
    mesh.clear();

    if (!CGAL::IO::read_polygon_mesh(path, mesh) || mesh.is_empty()) {
        std::cerr << "  Error: Failed to read mesh from '" << path << "'\n"
                  << "  Supported formats: .off  .obj  .ply  .stl\n";
        return false;
    }

    // Boolean operations require a triangle mesh
    if (!CGAL::is_triangle_mesh(mesh)) {
        std::cout << "  Triangulating non-triangle faces...\n";
        PMP::triangulate_faces(mesh);
    }

    if (!CGAL::is_closed(mesh))
        std::cerr << "  Warning: Mesh is not a closed solid — boolean result may be incorrect.\n";

    if (PMP::does_self_intersect(mesh))
        std::cerr << "  Warning: Mesh has self-intersections — boolean result may be incorrect.\n";

    return true;
}

// ----------------------------------------------------------------------------

static bool runBoolean(const std::string& op, Mesh& a, Mesh& b, Mesh& out) {
    if (op == "UNION")        return PMP::corefine_and_compute_union(a, b, out);
    if (op == "INTERSECTION") return PMP::corefine_and_compute_intersection(a, b, out);
    if (op == "DIFFERENCE")   return PMP::corefine_and_compute_difference(a, b, out);
    return false; // unreachable — validated by loadConfig
}

// ----------------------------------------------------------------------------

int main() {
    std::cout << "SPA-CGAL Boolean Operations\n"
              << "============================\n\n";

    std::string configPath;
    std::cout << "Config file path: ";
    std::getline(std::cin, configPath);

    // Strip surrounding quotes (common when dragging files onto a console window)
    if (configPath.size() >= 2 && configPath.front() == '"' && configPath.back() == '"')
        configPath = configPath.substr(1, configPath.size() - 2);

    Config cfg;
    try {
        cfg = loadConfig(configPath);
    } catch (const std::exception& e) {
        std::cerr << "\nConfig error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\nConfiguration\n"
              << "  Primary model  : " << cfg.primaryModel   << "\n"
              << "  Secondary model: " << cfg.secondaryModel << "\n"
              << "  Operation      : " << cfg.operation      << "\n"
              << "  Output path    : " << cfg.outputPath     << "\n\n";

    // ---- Load meshes -------------------------------------------------------
    Mesh meshA, meshB;

    std::cout << "Loading primary model...\n";
    if (!loadMesh(cfg.primaryModel, meshA)) return 1;
    std::cout << "  " << meshA.number_of_faces() << " faces, "
              << meshA.number_of_vertices() << " vertices\n\n";

    std::cout << "Loading secondary model...\n";
    if (!loadMesh(cfg.secondaryModel, meshB)) return 1;
    std::cout << "  " << meshB.number_of_faces() << " faces, "
              << meshB.number_of_vertices() << " vertices\n\n";

    // ---- Boolean operation -------------------------------------------------
    std::cout << "Computing " << cfg.operation << "...\n";
    Mesh result;
    if (!runBoolean(cfg.operation, meshA, meshB, result)) {
        std::cerr << "  Error: Boolean operation failed.\n"
                  << "  Ensure both meshes are valid, closed, manifold solids without self-intersections.\n";
        return 1;
    }
    std::cout << "  Result: " << result.number_of_faces() << " faces, "
              << result.number_of_vertices() << " vertices\n\n";

    // ---- Write output ------------------------------------------------------
    const fs::path outPath(cfg.outputPath);
    if (outPath.has_parent_path())
        fs::create_directories(outPath.parent_path());

    std::cout << "Writing '" << cfg.outputPath << "'...\n";
    if (!CGAL::IO::write_polygon_mesh(cfg.outputPath, result)) {
        std::cerr << "  Error: Failed to write output mesh.\n"
                  << "  Check that the path is writable and the extension is supported.\n";
        return 1;
    }
    std::cout << "  Done.\n";

    return 0;
}

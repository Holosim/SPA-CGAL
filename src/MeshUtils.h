#pragma once
#include <string>
#include <iostream>
#include <filesystem>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/IO/polygon_mesh_io.h>

namespace PMP = CGAL::Polygon_mesh_processing;
namespace fs  = std::filesystem;

using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using Mesh   = CGAL::Surface_mesh<Kernel::Point_3>;
using Point3 = Kernel::Point_3;
using Xform  = CGAL::Aff_transformation_3<Kernel>;

inline bool loadMesh(const std::string& path, Mesh& mesh, bool verbose = true) {
    mesh.clear();
    if (!CGAL::IO::read_polygon_mesh(path, mesh) || mesh.is_empty()) {
        std::cerr << "  Error: cannot read '" << path << "'\n"
                  << "  Supported formats: .obj  .off  .ply  .stl\n";
        return false;
    }
    if (!CGAL::is_triangle_mesh(mesh)) {
        if (verbose) std::cout << "  Triangulating '" << path << "'...\n";
        PMP::triangulate_faces(mesh);
    }
    if (!CGAL::is_closed(mesh))
        std::cerr << "  Warning: '" << path << "' is not closed — result may be incorrect.\n";
    if (PMP::does_self_intersect(mesh))
        std::cerr << "  Warning: '" << path << "' has self-intersections — result may be incorrect.\n";
    return true;
}

inline bool writeMesh(const std::string& path, Mesh& mesh) {
    fs::path p(path);
    if (p.has_parent_path())
        fs::create_directories(p.parent_path());
    if (!CGAL::IO::write_polygon_mesh(path, mesh)) {
        std::cerr << "  Error: cannot write '" << path << "'\n";
        return false;
    }
    return true;
}

// Unions 'next' into 'accumulator' in-place. Returns false on failure.
inline bool unionInto(Mesh& accumulator, Mesh& next) {
    Mesh result;
    if (!PMP::corefine_and_compute_union(accumulator, next, result))
        return false;
    accumulator = std::move(result);
    return true;
}

// Applies a 4x4 affine transform to every vertex of the mesh.
inline void applyTransform(Mesh& mesh, const Xform& xform) {
    for (auto v : mesh.vertices())
        mesh.point(v) = xform(mesh.point(v));
}

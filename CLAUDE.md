# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SPA-CGAL is a C++ console application that performs boolean operations (union, intersection, difference) on 3D mesh files using [CGAL](https://www.cgal.org/). It reads a configuration file specifying two input meshes and produces a single output mesh.

## Prerequisites

- **Visual Studio 2022** (Desktop C++ workload)
- **CMake 3.20+**
- **vcpkg** with `VCPKG_ROOT` environment variable set

Install dependencies via vcpkg (handled automatically at CMake configure time via `vcpkg.json`):
```
vcpkg install   # optional manual step; CMake will invoke this automatically
```

## Build

```powershell
# 1. Configure — generates SPA_CGAL.sln in ./build/
cmake --preset windows-vs2022-x64

# 2. Build
cmake --build --preset release

# Executable lands at: build/Release/BooleanOp.exe
```

To open in Visual Studio instead of building from the command line:
```
File > Open > CMake... → select CMakeLists.txt
```
VS will detect the preset automatically via `CMakePresets.json`.

## Running

Launch `BooleanOp.exe` from the repo root (paths in `config.ini` are relative to CWD):

```
build\Release\BooleanOp.exe
Config file path: sample/config.ini
```

The sample config runs a UNION of `sample/cube_a.off` (unit cube at origin) and `sample/cube_b.off` (cube offset by 0.5 on all axes), writing the result to `output/result.off`.

## Architecture

```
src/
  Config.h    — Config struct + loadConfig() INI parser (key=value, # comments)
  main.cpp    — Prompt → load config → load meshes → boolean op → write output
sample/
  config.ini  — Example configuration
  cube_a.off  — Unit cube [0,1]³ (triangulated OFF)
  cube_b.off  — Cube [0.5,1.5]³ (partially overlapping)
output/       — Written by BooleanOp.exe at runtime (gitignored except .gitkeep)
```

**Data flow in `main.cpp`:**
1. Read config path from stdin
2. `loadConfig()` parses key=value pairs (case-insensitive keys, strips `# ;` comments)
3. `CGAL::IO::read_polygon_mesh()` loads each mesh (dispatches on file extension)
4. `PMP::triangulate_faces()` ensures triangle-only meshes (required by corefinement)
5. Validity warnings: `CGAL::is_closed()`, `PMP::does_self_intersect()`
6. `PMP::corefine_and_compute_{union|intersection|difference}()` runs the op
7. `CGAL::IO::write_polygon_mesh()` writes output (format inferred from extension)

## Configuration File Format

```ini
# Lines starting with # or ; are comments
primary_model   = path/to/mesh_a.off
secondary_model = path/to/mesh_b.obj
operation       = UNION          # UNION | INTERSECTION | DIFFERENCE (default: UNION)
output_path     = output/out.off
```

Paths are relative to the working directory of the executable.

## CGAL Notes

- **Kernel**: `CGAL::Exact_predicates_inexact_constructions_kernel` (EPICK). Suitable for well-formed meshes. Switch to `CGAL::Exact_predicates_exact_constructions_kernel` (EPECK) for robustness with degenerate geometry — requires linking GMP/MPFR (included via vcpkg).
- **Corefinement requirement**: Both meshes must be closed, manifold, self-intersection-free triangle meshes. The program warns but does not abort on violations.
- **Supported I/O formats**: `.off`, `.obj`, `.ply`, `.stl` — determined by file extension.
- CGAL docs are pre-authorized for WebFetch: `doc.cgal.org`

# Stage 1 — Mission, Vision & Success Criteria

**Project:** Swept Path Analysis (SPA)
**Status:** Draft — awaiting review
**Last updated:** 2026-04-22

---

## 1. Problem Statement

When animating complex objects in 3D environments, artists and engineers have no immediate visual feedback on the total volume a moving object occupies across a span of time. Collision avoidance and motion-path refinement are currently manual, frame-by-frame processes:

- The animator positions an object, scrubs through the timeline, and eyeballs potential collisions
- Verifying clearance requires either inspecting many individual frames or running an expensive offline simulation
- Discovering a collision late in the process forces rework of already-refined animation curves

There is no tool in a standard Unreal Engine 5 workflow that automatically computes and displays the **swept path** — the aggregate 3D volume that an actor's mesh traces through space over a defined time interval — as animation data changes interactively.

---

## 2. Mission

Build a Swept Path Analysis (SPA) system that computes the 3D volume swept by a moving object's mesh over a configurable time window, and delivers that information to animators and engineers as an interactive, real-time visualization within their authoring environment.

---

## 3. Vision

A native Unreal Engine 5 **SPA Component** that any animator or technical director can attach to an actor in a scene. As they adjust keyframes, the component automatically recomputes the swept path model (SPM) — the CSG union of the actor's mesh at each sampled pose within the time window — and renders it as a semi-transparent overlay in the viewport. The time window is adjustable (e.g., "5 s before the playhead, 8 s after"), and the feature can be toggled off for performance when not needed.

The component is designed primarily for use **within the Unreal Editor itself**, updating the SPM overlay in real-time as animators scrub the timeline or adjust keyframes — without requiring Play-In-Editor (PIE) mode or a standalone runtime build. The visualization is equally valid during runtime (PIE or standalone), but the editor workflow is the primary use case.

The end result is that collision risks become **immediately visible** during the animation process, not after simulation, reducing iteration time and enabling higher-quality motion profiles.

---

## 4. Success Criteria (v1.0)

The following criteria define "done" for the initial production release of the SPA system. Detailed acceptance thresholds are established in Stage 4 (V&V Strategy).

| ID | Criterion | Category |
|----|-----------|----------|
| SC-01 | The SPM accurately represents the CSG union of the actor mesh at every sampled pose within the configured time window | Correctness |
| SC-02 | The SPA Component functions and updates within the Unreal Editor without requiring PIE mode or a standalone runtime build | Integration |
| SC-03 | The SPA Component attaches to any Actor in a UE5 scene without modifying the actor's Blueprint or class | Integration |
| SC-04 | All component controls — including enable/disable toggle, pre-frame duration, post-frame duration, and sample interval — are accessible and functional in Editor mode without PIE | Usability |
| SC-05 | The component can be toggled on/off without crashing or corrupting the scene in either Editor or PIE mode | Stability |
| SC-06 | The SPM updates within an acceptable latency budget when keyframes are modified (target defined in Stage 4) | Performance |
| SC-07 | The system handles production-scale meshes (target poly count defined in Stage 3) without exceeding memory budget | Performance |
| SC-08 | The SPM geometry is valid (closed, manifold, no self-intersections) or degrades gracefully with a visible warning | Robustness |
| SC-09 | All geometry processing is encapsulated behind a well-defined interface; the CGAL backend can be replaced by an alternative library (e.g., OpenVDB, OCCT) without redesigning the SPA component | Architecture |
| SC-10 | The component displays on-screen warnings in the Editor for: (a) resource limits reached / dynamic quality degradation active, (b) unsupported mesh type attached (soft-body, cloth, fluid body, non-affine rigged mesh); warnings remain visible for 15 seconds and are also logged to the Output panel | Usability |
| SC-11 | The standalone CGAL CLI tool (current prototype) continues to function as a reproducible offline reference implementation | Compatibility |

---

## 5. Roadmap Items (v2.0)

The following features are explicitly deferred to a post-v1.0 release and should be kept in mind during architecture to avoid being designed out.

**Dynamic quality management:** The component monitors system resource utilization and automatically reduces the time window duration and/or sample interval when resources approach a defined lower threshold, then continuously attempts to restore the configured values as resources become available. Requirements for this feature:
- Resource monitoring must impose negligible overhead (e.g., check interval on the order of seconds, not per-frame)
- The reduction strategy must be deterministic and reversible (no permanent state change)
- The user's configured values are preserved and treated as the target; degraded values are transient
- SC-10 warning (a) applies during any degraded state

---

## 6. Out of Scope (v1.0)

- Swept paths for multiple actors simultaneously within a single component instance
- Soft-body, cloth, or fluid deformation during the sweep (rigid-body transforms only)
- Physics simulation integration (SPA reads animation transforms, not physics state)
- Non-affine deformations (skeletal mesh deformation is excluded; only the actor's root transform is swept)
- Automated collision reporting or resolver — SPA visualizes; it does not fix
- Export of the SPM as a persistent asset in the UE5 content browser (visualization only in v1.0)
- Dynamic quality management (deferred to v2.0 — see Section 5)

---

## 7. Technology Landscape

The geometry processing backend is the highest-risk component of this system. Research conducted prior to architecture selection is captured in `docs/GeometryStackResearch_TightFitShellModeling.md`. The candidate libraries are summarized below; final selection occurs in Stage 8 (Architecture) and is validated in Stage 11 (Architectural Spike).

| Library | Role | Approach | Real-time Suitability | License |
|---------|------|----------|----------------------|---------|
| **CGAL** | Mesh CSG backend (current prototype) | Corefinement-based mesh boolean union; exact predicates/constructions | Moderate — consecutive exact unions are heavy; requires adaptive sampling, broad-phase pruning, parallel tree reduction, and periodic simplification for near-RT | Dual GPL/LGPL + commercial |
| **Open CASCADE (OCCT)** | CAD B-Rep kernel; STEP I/O; healing | B-Rep boolean fuse; native sweep/revolve operators; tolerant modeling | Moderate — parallel boolean flags exist; same fundamental scaling concern as CGAL under many samples | LGPL 2.1 |
| **OpenVDB** | Volumetric / level-set CSG backend | Sparse SDF union; threaded CSG operations | High — best near-RT option; approximate at chosen voxel resolution; topology-stable under many poses | Apache 2.0 |
| **Eigen** | Kinematics / transform math | Linear algebra, SE(3) transforms | High — CPU-vectorized; not a CSG engine | MPL 2.0 |
| **Embree** | Spatial query acceleration | Ray-based intersection / broad-phase pruning | High — useful to test whether a new pose even intersects the current SPM boundary before triggering a full union | Apache 2.0 |
| **Parasolid / ACIS** | Commercial CAD kernels | Industrial-grade tolerant B-Rep booleans | Moderate — multi-processor support documented; same sample-count scaling concern | Proprietary |

**Ghosting as a degraded-mode fallback:** The UE5 PCG-based ghosting technique (documented in `docs/unreal_engine_ghosting_blueprint_guide.md`) renders individual transparent mesh instances at each sampled pose without computing a CSG union. This is orders of magnitude cheaper and may serve as the v2.0 dynamic-quality fallback when resources are insufficient for full SPM computation — showing the individual pose "ghost" silhouettes rather than the true swept volume boundary.

---

## 8. Risks Identified at This Stage

| ID | Risk | Likelihood | Impact | Mitigation |
|----|------|-----------|--------|------------|
| R-01 | CGAL corefinement ops at 50k+ triangle meshes are too slow for interactive Editor update rates | **High** | High | Research confirms this explicitly. Quantify target latency in Stage 4; design swappable backend in Stage 8; spike OpenVDB and CGAL side-by-side in Stage 11 |
| R-02 | Switching from EPICK to EPECK kernel (required for robust consecutive unions) may add unacceptable compute cost | High | High | Profile EPICK vs EPECK on representative meshes during Stage 11 spike; OpenVDB fallback avoids this trade-off entirely |
| R-03 | UE5 C++ plugin build system is incompatible with CGAL or OCCT third-party linkage | Medium | High | Prototype build integration early in Stage 11 before committing to a geometry backend |
| R-04 | Highly complex or non-manifold production meshes cause silent invalid output from the geometry backend | Medium | Medium | Define mesh validity requirements in Stage 3; add validation layer with warnings (feeds SC-10) |
| R-05 | N consecutive pose unions cause accumulated mesh complexity to grow without bound, exceeding memory budget | Medium | High | Stage 8 must specify mesh simplification / decimation cadence; periodic remesh is a standard mitigation in the literature |
| R-06 | Editor-mode real-time update requires UE5 Editor subsystem hooks that may not support per-keyframe callbacks at the required frequency | Medium | Medium | Investigate UE5 Editor animation notification APIs in Stage 5 (System Boundaries); prototype hook in Stage 11 |

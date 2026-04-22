# Stage 4 — V&V Strategy & Acceptance Criteria

**Project:** Swept Path Analysis (SPA)
**Status:** Draft — awaiting review
**Last updated:** 2026-04-22

---

## 1. V&V Philosophy

Validation confirms the system does the right thing (meets the mission and user needs). Verification confirms the system does the thing right (meets the specified requirements). Both are required; neither replaces the other.

Three guiding principles for SPA:

1. **Test against requirements, not against implementation.** Tests are written from FR/NFR specifications, not from knowledge of how the code works internally. A test that only passes because of a specific CGAL call is not a requirement test — it is an implementation test.

2. **Performance targets are preliminary until Stage 11.** The quantified targets in this document (latency, memory) are engineering estimates based on C-06 design parameters. The Stage 11 architectural spike is the first hard data point. Targets may be revised after that spike if the estimates are materially wrong — but revision requires explicit sign-off, not silent adjustment.

3. **Safety-critical accuracy is non-negotiable.** NFR-01 (no aliasing gaps in the SPM) is a hard requirement, not a quality target. A system that computes swept volumes with undetected gaps is not a degraded SPA — it is a broken SPA. The V&V plan treats NFR-01 differently from performance targets as a result.

---

## 2. Test Levels and Types

| Level | Type | Description | When Executed |
|-------|------|-------------|---------------|
| L1 | **Unit** | Individual functions tested in isolation (leading-edge classifier, bridge face generator, keyframe range calculator, buffer swap logic) | Stage 12 (continuous) |
| L2 | **Integration** | Stroke Layer cache + geometry backend together; UE5 component + animation asset together; display buffer + material system together | Stage 12–13 (continuous) |
| L3 | **System** | Full end-to-end: actor in UE5 Editor → keyframe edit → SPM overlay update → visual verification | Stage 13–14 |
| L4 | **Performance** | Latency measurement, memory profiling, thread contention analysis at C-06 parameters and above | Stage 11 (spike), Stage 14 (formal) |
| L5 | **Visual Acceptance** | Manual inspection of SPM accuracy, staleness color behavior, absence of flickering; cannot be fully automated | Stage 13–14 |
| L6 | **Regression** | CLI tool output matches known-good reference; repeated identical keyframe edits produce geometrically identical SPM output | Stage 12 (continuous) |
| L7 | **Fault Injection** | Geometry backend returns errors; resource limits are artificially hit; animation API returns malformed data | Stage 13 |

---

## 3. Quantified Acceptance Criteria

This section resolves all **[TBD]** values from Stage 3 and answers the open questions OQ-01 through OQ-09.

### 3.1 Performance Targets (NFR-03, NFR-04, NFR-11)

> These are **preliminary engineering estimates** based on C-06 parameters. They are binding for acceptance testing at Stage 14 but are subject to revision after the Stage 11 spike produces real benchmark data. Any revision requires explicit change control.

| NFR | Target | Rationale |
|-----|--------|-----------|
| NFR-03 (Update Latency — single keyframe change) | ≤ 3 seconds end-to-end (keyframe edit event → buffer swap complete) at C-06 parameters | A 3-second delay is perceptible but not workflow-breaking for an animator making incremental adjustments. The Stroke Layer cache (FR-14–FR-18) should keep most single-keyframe changes well under this. |
| NFR-03 (Update Latency — full-sequence invalidation) | ≤ 30 seconds at C-06 parameters | A complete re-sweep of 7,200 samples is a cold-start rebuild; 30 s is acceptable for an initial load or a wholesale retiming of the entire sequence. |
| NFR-04 (Stroke Layer cache memory) | ≤ 256 MB total for all cache layers at C-06 parameters | Chosen to remain a minor fraction of a typical production workstation's available RAM (16–64 GB). Includes all Layer-0 through Layer-3 mesh segments and both display buffer slots. |
| NFR-11 (Disabled overhead) | ≤ 0.05 ms average per-tick | At 60 fps (16.7 ms/frame), this represents < 0.3% of frame budget — effectively unmeasurable in practice. |

### 3.2 Accuracy Tolerance (NFR-01)

| Check | Criterion |
|-------|-----------|
| Geometric completeness | For any motion with an analytically computable swept volume (e.g., a convex brush translated along a straight path), all exterior surface vertices of the expected swept volume shall be present in the SPM output within a positional tolerance of **0.1 mm** (0.0001 UE units, where 1 UE unit = 1 cm by convention). |
| No aliasing gaps | For any test motion, no line segment connecting any point on the brush at time tᵢ to its corresponding point at tᵢ₊₁ shall pass through the exterior of the output SPM. This is the formal definition of "no aliasing gap." |
| Topological completeness | The SPM shall be a single connected component for any continuous motion path that begins and ends in a non-self-intersecting configuration. |

### 3.3 Output Mesh Quality Criteria (NFR-02)

All of the following must pass on the final Layer-0 SPM mesh:

- `CGAL::is_closed(mesh)` returns true
- `CGAL::is_triangle_mesh(mesh)` returns true
- `PMP::does_self_intersect(mesh)` returns false
- Zero faces with area < 1e-10 (degenerate/zero-area faces)
- Zero duplicate vertices (within positional tolerance 1e-6)

If any check fails, the component must emit a warning (FR-32) and continue displaying the last valid SPM (FR-41).

### 3.4 Default Parameter Values (OQ-04, OQ-05, OQ-09)

These defaults are the starting values for implementation. All are exposed as configurable properties unless noted otherwise.

| Parameter | Default | Notes |
|-----------|---------|-------|
| Leading-edge epsilon (FR-07) | **0.01** (dot product, dimensionless) | Adjustable range: [0.0001, 0.1]. Calibrated empirically in Stage 11 on reference motions. |
| Decimation trigger threshold (FR-23) | Triangle count > **10× source brush polygon count** OR every **50 Layer-3 intervals**, whichever comes first | Prevents unbounded growth while limiting overhead frequency. Reviewed in Stage 11 benchmarks. |
| Default normal SPM color (FR-44) | `FLinearColor(0.15, 0.55, 1.0, 0.35)` — sky blue, 35% opacity | Chosen for visibility against typical scene geometry without obscuring the underlying actor. |
| Default stale SPM color (FR-43) | `FLinearColor(1.0, 0.45, 0.0, 0.50)` — orange, 50% opacity | Distinct from the normal color and from UE5's standard error/collision indicators (red). |
| Resource warning threshold (FR-31, OQ-08) | CPU utilization > **80%** sustained for > **3 seconds**, OR free system RAM < **512 MB** | Conservative thresholds; intended to warn before instability, not after. |
| Resource monitor interval (FR-V2-03) | **5 seconds** | Low-frequency polling to impose negligible overhead. |

### 3.5 Display Buffering & Staleness (SC-12, SC-13, FR-40–FR-44)

| Test | Criterion |
|------|-----------|
| No-flicker during update | Frame-by-frame analysis of a screen recording during a keyframe edit shall show zero frames where the SPM overlay is absent, blank, or partially rendered |
| Stale tint onset | The overlay material shall adopt the stale color within **1 render tick** (≤ 16.7 ms at 60 fps) of a keyframe invalidation event being detected |
| Normal color restoration | The overlay material shall revert to the normal color within **1 render tick** of the buffer swap completing |
| Buffer swap atomicity | No intermediate state between the old SPM and the new SPM shall be visible at any point during the swap |

---

## 4. V&V Matrix

Maps each FR group and NFR to its test level(s), test IDs, acceptance criteria, and execution stage.

### 4.1 Functional Requirements

| FR Group | Test Level | Test ID(s) | Acceptance Criterion | Execution Stage |
|----------|-----------|------------|---------------------|-----------------|
| FR-01–FR-05 (Animation Data Access) | L2, L3 | T-AD-01: Load UStaticMesh asset; T-AD-02: Read UAnimSequence at arbitrary times; T-AD-03: Load FBX-imported asset end-to-end | Mesh vertices match source asset; transform data at sampled times matches UE5 animation evaluation output | Stage 12 |
| FR-06–FR-13 (Leading-Edge Extrusion) | L1, L6 | T-LE-01: Unit test classifier on known vertex/normal/displacement combos; T-LE-02: Bridge face winding test; T-LE-03: Backtracking motion regression | Classifier output matches manual calculation; bridge faces are watertight; backtracking produces no self-intersections | Stage 12 |
| FR-14–FR-19 (Stroke Layer Cache) | L1, L2 | T-SL-01: Invalidation range calculation unit test; T-SL-02: Bottom-up regeneration integration test; T-SL-03: Cache hit rate measurement (valid segments not recomputed) | Invalidation range matches expected union of channel intervals; regeneration produces correct Layer-0; zero unnecessary recomputation | Stage 12 |
| FR-20–FR-23 (Boolean Union & Post-Processing) | L2, L4 | T-BU-01: Union of two known prism volumes produces correct mesh; T-BU-02: Post-union mesh quality checks (NFR-02 criteria); T-BU-03: Parallel execution produces same result as serial | NFR-02 mesh quality criteria pass; parallel result is geometrically identical to serial result | Stage 12, 14 |
| FR-24–FR-29 (UE5 Component Integration) | L3, L5 | T-UE-01: Attach component to Actor in Editor; T-UE-02: Properties visible in Details panel without PIE; T-UE-03: Overlay renders in viewport without PIE; T-UE-04: Toggle disables computation | Component attaches without modifying Actor; all properties visible; overlay appears within latency target; zero CPU tick cost when disabled | Stage 13 |
| FR-30–FR-33 (Warnings & Diagnostics) | L3, L7 | T-WD-01: Attach to unsupported mesh type; T-WD-02: Inject resource limit condition; T-WD-03: Inject geometry backend error | Warning appears on-screen for 15 s and in Output Log; component remains stable; Output Log entry written at update start | Stage 13 |
| FR-40–FR-44 (Display Buffering & Staleness) | L3, L5 | T-DB-01: Screen recording during keyframe edit (no-flicker check); T-DB-02: Stale color onset timing; T-DB-03: Normal color restoration timing; T-DB-04: Buffer swap atomicity | All criteria from §3.5 pass | Stage 13 |
| FR-34–FR-36 (Geometry Backend Interface) | L2 | T-GB-01: Register mock backend; run full integration test suite; confirm zero changes to USPAComponent | All integration tests pass with mock backend; `git diff` on USPAComponent shows zero changes | Stage 12 |
| FR-37–FR-39 (CLI Reference Tool) | L6 | T-CLI-01: Run BooleanOp.exe on reference inputs; compare output to known-good mesh; T-CLI-02: Benchmark output matches baseline | Output mesh passes NFR-02 quality checks; benchmark values within 10% of committed baseline | Stage 12 (continuous) |

### 4.2 Non-Functional Requirements

| NFR | Test Level | Test ID(s) | Acceptance Criterion | Execution Stage |
|-----|-----------|------------|---------------------|-----------------|
| NFR-01 (Accuracy) | L1, L5, L6 | T-ACC-01: Straight translation sweep vs. analytical volume; T-ACC-02: Rotation sweep gap check; T-ACC-03: Aliasing line-segment test | See §3.2 criteria | Stage 11 (spike), 14 (formal) |
| NFR-02 (Mesh Quality) | L2, L6 | T-MQ-01: Full CGAL validity suite on all test outputs | All CGAL checks pass; zero degenerate faces | Stage 12 (continuous) |
| NFR-03 (Update Latency) | L4 | T-PERF-01: Single-keyframe change latency at C-06; T-PERF-02: Full-sequence invalidation latency at C-06 | ≤ 3 s single-keyframe; ≤ 30 s full-sequence | Stage 11 (estimate), 14 (acceptance) |
| NFR-04 (Memory Budget) | L4 | T-MEM-01: Peak RSS measurement during full sweep at C-06 using Windows Performance Monitor | All cache layers ≤ 256 MB total | Stage 11 (estimate), 14 (acceptance) |
| NFR-06 (Backend Modularity) | L2 | T-GB-01 (shared with FR-34–FR-36) | See FR-34–FR-36 row above | Stage 12 |
| NFR-07 (Thread Safety) | L4 | T-TS-01: Run parallel Layer-3 generation under Thread Sanitizer with 8 threads, 100 iterations | Zero data races, deadlocks, or cache corruption detected | Stage 12–13 |
| NFR-08 (Graceful Degradation) | L7 | T-GD-01: Fault injection — backend returns error on segment N; T-GD-02: Animation API returns malformed transform | Last valid SPM remains visible; warning logged; component does not crash; scene state unchanged | Stage 13 |
| NFR-09 (Editor Compatibility) | L3 | T-UE-01 through T-UE-04 (shared) | SPM updates in Editor without PIE on UE5.4+ | Stage 13 |
| NFR-10 (Build Isolation) | L3 | T-BUILD-01: Clean UE5.4 install + plugin source → cmake + build | Zero engine source modifications; build succeeds with vcpkg deps only | Stage 11 |
| NFR-11 (Disabled Overhead) | L4 | T-PERF-03: UE5 Insights tick profiling with Enable SPM Overlay = false | ≤ 0.05 ms average per-tick attributed to SPA component | Stage 14 |
| NFR-12 (Topology Stability) | L6 | T-REG-01: Edit same keyframe 50× in succession; diff all output meshes | All 50 output meshes geometrically identical within 1e-6 tolerance; no degradation in closed/manifold/self-intersection metrics | Stage 13 |

---

## 5. Test Environment & Tooling

| Component | Specification |
|-----------|--------------|
| **OS** | Windows 11 x64 (primary); cross-platform correctness checks on Linux if CI is available |
| **Engine** | Unreal Engine 5.4 or later |
| **Compiler** | Visual Studio 2022 with MSVC v143 toolchain |
| **Geometry library** | CGAL (version pinned via vcpkg.json) |
| **Unit test framework** | Google Test (gtest) for standalone C++ unit tests outside UE5; UE5 Automation Framework for in-engine tests |
| **Performance profiling** | UE5 Insights (tick budget, memory); Windows Performance Monitor (RSS, peak working set); custom `BenchmarkRow` instrumentation (existing in CLI tool) |
| **Thread safety analysis** | Thread Sanitizer (TSAN) via Clang/GCC on Linux CI; Visual Studio Concurrency Analyzer on Windows |
| **Visual verification** | OBS Studio or Windows Game Bar screen recording; manual frame-by-frame review for flicker tests |
| **Mesh validation** | CGAL `is_closed()`, `is_triangle_mesh()`, `PMP::does_self_intersect()` invoked post-output in all automated tests |
| **Reference data** | `sample/cube_a.off`, `sample/cube_b.off`, `sample/model/brush.obj`, `sample/transforms.csv` (existing in repo) + additional reference motions to be added in Stage 12 |

---

## 6. Open Questions Resolved

All OQ items from Stage 3 are resolved as of this stage:

| OQ | Resolution |
|----|-----------|
| OQ-01 (latency target) | ≤ 3 s single-keyframe; ≤ 30 s full-sequence at C-06 (§3.1) |
| OQ-02 (memory budget) | ≤ 256 MB total Stroke Layer cache at C-06 (§3.1) |
| OQ-03 (disabled overhead) | ≤ 0.05 ms/tick (§3.1) |
| OQ-04 (epsilon default) | 0.01, range [0.0001, 0.1] (§3.4) |
| OQ-05 (decimation cadence) | Triangle count > 10× source polygon count OR every 50 intervals (§3.4) |
| OQ-06 (UE5 keyframe notification API) | Remains open — investigation deferred to Stage 5 (System Boundaries) and Stage 11 spike. Test T-UE-03 will fail if no suitable API exists, forcing a design decision. |
| OQ-07 (UE5 animation evaluation efficiency) | Remains open — investigation deferred to Stage 11 spike. Test T-PERF-01 will surface this. |
| OQ-08 (resource threshold definition) | CPU > 80% sustained > 3 s OR free RAM < 512 MB (§3.4) |
| OQ-09 (default hue colors) | Normal: FLinearColor(0.15, 0.55, 1.0, 0.35); Stale: FLinearColor(1.0, 0.45, 0.0, 0.50) (§3.4) |

OQ-06 and OQ-07 remain open. Their resolution is a Stage 11 dependency; the architecture in Stage 8 must account for both a push-notification path and a polling fallback.

---

## 7. Risks Identified at This Stage

| ID | Risk | Likelihood | Impact | Mitigation |
|----|------|-----------|--------|------------|
| R-13 | Stage 11 spike reveals the 3-second latency target (NFR-03) is unachievable with CGAL at C-06 parameters even with the Stroke Layer cache | Medium | High | OpenVDB backend spike runs in parallel during Stage 11; if CGAL misses target, OpenVDB becomes the primary backend candidate for v1.0 |
| R-14 | Thread Sanitizer tests (T-TS-01) reveal CGAL is not safe for concurrent use in the parallel stroke generation path (FR-21) | High | Medium | Per-thread CGAL instance isolation is the standard mitigation; flag this as a mandatory architecture constraint in Stage 8 |
| R-15 | Visual acceptance tests (T-DB-01 flicker test) cannot be automated reliably and become a bottleneck in the validation cycle | Low | Low | Maintain a manual test runbook for L5 visual tests; automate the color-onset timing tests (T-DB-02, T-DB-03) separately via programmatic material parameter inspection |

# Stage 3 — Requirements (Functional + Non-Functional)

**Project:** Swept Path Analysis (SPA)
**Status:** Draft — awaiting review
**Last updated:** 2026-04-22

---

## 1. Constraints

Constraints are fixed conditions the system must operate within. They are not negotiable requirements — they are boundary conditions set by platform, technology, or project decisions already made.

| ID | Constraint | Source |
|----|-----------|--------|
| C-01 | Implementation language: C++ with UE5 plugin architecture | Platform decision |
| C-02 | Primary platform: Windows (UE5 Editor); cross-platform architecture is encouraged where it does not add cost | Platform decision |
| C-03 | Animation data sources in v1.0: UE5 native internal formats (UStaticMesh, UAnimSequence) and FBX-imported assets; no Assimp, no Live Link | Stage 1 decision |
| C-04 | Geometry backend in v1.0: CGAL; must be accessed through a swappable interface | Stage 1 decision |
| C-05 | The SPA plugin shall not require modification of UE5 engine source; all integration through public plugin APIs | Platform constraint |
| C-06 | **Minimum design parameters** (the baseline all performance requirements are measured against): brush mesh ≤ 150 polygons; sample rate = 30 samples/sec; animation duration = 4 minutes → 7,200 total samples | Algorithm doc |

---

## 2. Functional Requirements

Functional requirements define *what* the system must do. Requirements marked **[v2.0]** are deferred to a post-v1.0 release; they are captured here to ensure the v1.0 architecture does not design them out.

### 2.1 Animation Data Access

| ID | Requirement |
|----|------------|
| FR-01 | The system shall read static mesh geometry (vertices, faces, normals) from UE5 `UStaticMesh` assets |
| FR-02 | The system shall read per-frame world-space transform data from UE5 `UAnimSequence` assets, evaluating the animation at arbitrary time positions |
| FR-03 | The system shall accept FBX-imported mesh and animation assets as a primary source (i.e., no additional conversion step required beyond the standard UE5 FBX import pipeline) |
| FR-04 | The system shall sample animation transforms at a user-configurable rate; default rate shall be 30 samples/sec; minimum supported rate shall be 1 sample/sec |
| FR-05 | The animation data provider shall be accessed through an interface that can be extended in future releases to support additional sources (e.g., Live Link) without modifying the SPA component |

### 2.2 Leading-Edge Extrusion Algorithm

| ID | Requirement |
|----|------------|
| FR-06 | For each sample interval [tᵢ, tᵢ₊₁], the system shall compute the world-space displacement vector **V** for each vertex: **V** = Vertex_position(tᵢ₊₁) − Vertex_position(tᵢ) |
| FR-07 | The system shall classify each vertex as a **leading-edge vertex** when the projection of **V** onto the vertex's world-space normal exceeds a configurable epsilon threshold ε (default value TBD in Stage 4/11) |
| FR-08 | Vertices where the projection is less than or equal to ε (trailing or stationary) shall be excluded from the extrusion step for that interval |
| FR-09 | For each interval, the system shall collect the set of **leading-edge polygons**: polygons for which at least one vertex is a leading-edge vertex |
| FR-10 | The system shall generate **bridge faces** connecting the leading-edge polygon vertices at tᵢ (reversed winding) and tᵢ₊₁ (preserved winding) to form a closed, watertight prism volume for that interval |
| FR-11 | The system shall handle the case where a vertex's projection is near zero (within ε) without producing degenerate bridge faces; near-zero vertices shall be treated as stationary for that interval |
| FR-12 | The system shall handle **backtracking motion** — intervals where the brush returns into previously occupied space — without producing self-intersecting or topologically invalid prism volumes |
| FR-13 | After each interval's prism volume is merged into the accumulating stroke, the system shall apply **per-step topology simplification**: merge near-duplicate vertices, remove zero-area or degenerate faces |

### 2.3 Stroke Layer Cache System

| ID | Requirement |
|----|------------|
| FR-14 | The system shall maintain a **four-level hierarchical stroke cache** of pre-computed stroke mesh segments: Layer 0 (full timeline), Layer 1 (bisected halves), Layer 2 (user-defined regular intervals), Layer 3 (per-keyframe-interval segments, finest grain) |
| FR-15 | The system shall maintain a parallel **Time Layer index** tracking animation keyframes per animated channel (Translate X/Y/Z, Rotate X/Y/Z, Scale) to define Layer-3 segment boundaries |
| FR-16 | On any keyframe modification, the system shall compute the **invalidation range** as the union of surrounding keyframe intervals across all animated channels for the modified object |
| FR-17 | The system shall invalidate all Layer-3 stroke segments whose time ranges overlap the invalidation range, and propagate invalidation upward through Layers 2, 1, and 0 |
| FR-18 | The system shall regenerate invalidated segments **bottom-up**: Layer 3 → Layer 2 → Layer 1 → Layer 0; valid (non-invalidated) cache segments shall be reused without recomputation |
| FR-19 | Layer-0 shall be the only stroke visible in the scene; Layer 1 through Layer 3 segments shall be stored in memory (not rendered) |

### 2.4 Boolean Union & Post-Processing

| ID | Requirement |
|----|------------|
| FR-20 | The system shall perform a **CSG boolean union** between each new prism volume and the accumulated stroke for each Layer-3 segment, using the active geometry backend (FR-29) |
| FR-21 | The system shall support **parallel execution** of Layer-3 stroke segment generation across multiple CPU threads; all shared cache state must be protected for thread safety (see NFR-07) |
| FR-22 | After the full stroke is regenerated (Layer 0 complete), the system shall apply **post-union cleanup**: heal degenerate faces, stitch small holes, merge near-duplicate vertices |
| FR-23 | The system shall apply **mesh decimation / remeshing** to reduce polygon density in over-dense regions of the final stroke; decimation aggressiveness shall be a configurable parameter |

### 2.5 UE5 Component Integration

| ID | Requirement |
|----|------------|
| FR-24 | The SPA system shall be implemented as a UE5 **Actor Component** (`USPAComponent`) attachable to any Actor in a scene without modifying the Actor's class or Blueprint |
| FR-25 | The component shall expose the following properties in the **Details panel**, accessible in Editor mode without PIE: Enable SPM Overlay (bool), Pre-Frame Duration (float, seconds), Post-Frame Duration (float, seconds), Sample Interval (float, seconds) |
| FR-26 | The component shall render the SPM as a **semi-transparent procedural mesh overlay** in the UE5 Editor viewport using a `UProceduralMeshComponent` or equivalent |
| FR-27 | The component shall update the SPM overlay in the **Editor without requiring PIE**, responding to keyframe changes in the Sequencer or Animation Editor |
| FR-28 | When the Enable SPM Overlay property is **false**, the component shall perform no SPM computation and shall consume negligible CPU and memory resources per tick |
| FR-29 | The component shall function equivalently during PIE and standalone runtime modes; Editor mode is the primary use case |

### 2.6 SPM Display Buffering & Staleness Visualization

| ID | Requirement |
|----|------------|
| FR-40 | The SPA component shall maintain a **display buffer** and an **update buffer** for the Layer-0 SPM mesh: the display buffer contains the currently visible SPM; the update buffer receives the newly computed SPM during background recomputation |
| FR-41 | The display buffer shall remain visible and unmodified during any background recomputation; the overlay mesh shall not be removed, hidden, or partially updated until the new update buffer is fully computed and validated |
| FR-42 | When background recomputation completes successfully, the system shall **atomically swap** the update buffer into the display buffer position before releasing the previous display buffer, eliminating any blank or flickering interval in the viewport |
| FR-43 | When the display buffer contains a Layer-0 SPM that is **stale** (one or more Stroke Layer segments have been invalidated and recomputation is in progress), the overlay material shall apply a configurable **hue shift** to visually communicate the out-of-date state; the default stale hue shall be a red/orange tint |
| FR-44 | When the buffer swap completes (FR-42), the overlay material shall revert to the **normal SPM color**; both the stale color and the normal color shall be exposed as configurable `FLinearColor` properties in the Details panel |

### 2.7 Warnings & Diagnostics

| ID | Requirement |
|----|------------|
| FR-30 | The component shall detect when the attached actor uses an **unsupported mesh type** (soft-body, cloth, fluid body, or mesh rigged for non-affine transformations) and shall disable SPM computation for that actor |
| FR-31 | The component shall detect when **system resource utilization** reaches a user-configurable lower threshold and shall post a resource limit warning (v1.0: warning and display only; dynamic quality reduction is deferred to v2.0) |
| FR-32 | All warnings (FR-30, FR-31, and any geometry backend errors) shall: (a) display on-screen in the Editor viewport for 15 seconds, and (b) be written to the UE5 **Output Log** panel simultaneously |
| FR-33 | The component shall write a **"Recomputing…" status entry** to the Output Log when an SPM update begins; the primary in-viewport visual cue for the recomputing state is the staleness hue shift (FR-43), not a text overlay |

### 2.8 Geometry Backend Interface

| ID | Requirement |
|----|------------|
| FR-34 | All geometry processing operations — boolean union, topology simplification, mesh cleanup, and decimation — shall be invoked exclusively through a defined C++ abstract interface (`ISPAGeometryBackend` or equivalent) |
| FR-35 | The CGAL-based implementation (`SPACGALBackend`) shall be the default backend registered at plugin startup in v1.0 |
| FR-36 | An alternative backend (e.g., OpenVDB, OCCT) shall be substitutable by implementing `ISPAGeometryBackend` and updating the backend registration point, with **no changes required** to `USPAComponent`, the Editor integration layer, or any UE5 Blueprint nodes |

### 2.9 CLI Reference Tool

| ID | Requirement |
|----|------------|
| FR-37 | The standalone CGAL CLI tool (`BooleanOp.exe`) shall remain buildable and functional as an **offline reference implementation** across all future development |
| FR-38 | The CLI tool shall accept a config file specifying a base mesh path, a transforms CSV path, and an output path, and shall produce a valid, closed SPM mesh file |
| FR-39 | The CLI tool shall report per-step benchmark data (step duration, cumulative duration, face/vertex counts, peak memory) to the console for use as a performance regression baseline |

### 2.10 Deferred — v2.0 Functional Requirements

| ID | Requirement |
|----|------------|
| FR-V2-01 | When resource utilization exceeds a configured threshold, the component shall **automatically reduce** Pre-Frame Duration, Post-Frame Duration, and/or Sample Interval to lower computation load |
| FR-V2-02 | The component shall **continuously attempt to restore** the user-configured values as resources become available |
| FR-V2-03 | Resource monitoring shall execute at a **low-frequency interval** (configurable; default: every 5 seconds) to impose negligible overhead |
| FR-V2-04 | The component shall support **Live Link** as an optional animation data source, implemented as a second `IAnimationDataProvider` without modifying core SPA logic |

---

## 3. Non-Functional Requirements

Non-functional requirements define *how well* the system must perform. Values marked **[TBD]** are to be quantified in Stage 4 (V&V Strategy & Acceptance Criteria).

| ID | Category | Requirement |
|----|----------|------------|
| NFR-01 | **Accuracy** | The SPM shall represent the complete swept volume boundary with no aliasing gaps between time samples for rigid-body, affine-transform motion. Leading-edge extrusion geometry shall be topologically equivalent to the true swept volume. No collision-relevant geometry shall be absent from the SPM. |
| NFR-02 | **Output Mesh Quality** | The final SPM (Layer 0) shall be a closed, manifold, triangle mesh with no self-intersections, no degenerate faces, and no zero-area polygons. Violations shall trigger a warning (FR-32) rather than producing silently invalid output. |
| NFR-03 | **Update Latency** | The SPM overlay shall complete its update cycle within **[TBD]** seconds of a keyframe modification, at the minimum design parameters (C-06). The Stroke Layer cache (FR-14 through FR-18) is the primary latency mitigation. Preliminary target: ≤ 3 seconds for a single-keyframe change on C-06 parameters. |
| NFR-04 | **Memory Budget** | The combined memory footprint of all Stroke Layer cache segments shall not exceed **[TBD]** MB at C-06 parameters. Preliminary guidance: should remain within a budget that does not materially impact a production UE5 scene's available memory. |
| NFR-05 | **Scale Target** | The system shall meet all NFRs at the minimum design parameters defined in C-06. Performance at larger meshes or longer animations is aspirational and subject to Stage 11 benchmarking. |
| NFR-06 | **Backend Modularity** | Replacing the geometry backend implementation shall require zero modifications to `USPAComponent`, the Editor integration layer, the Stroke Layer cache system, or any UE5 Blueprint bindings. |
| NFR-07 | **Thread Safety** | All reads and writes to shared Stroke Layer cache state shall be thread-safe. Parallel Layer-3 segment generation (FR-21) shall not produce data races, deadlocks, or cache corruption under any interleaving of thread execution. |
| NFR-08 | **Graceful Degradation** | If the geometry backend returns an error or invalid mesh for any stroke segment, the system shall retain the last valid SPM, emit a warning (FR-32), and continue operating. The component shall not crash, throw unhandled exceptions, or corrupt UE5 scene state. |
| NFR-09 | **Editor Compatibility** | The SPA plugin shall compile and function correctly against **UE5.4+** without engine source modification. The SPM overlay shall update live in the Editor viewport without PIE. |
| NFR-10 | **Build Isolation** | All third-party library linkage (CGAL and future backends) shall be managed through the UE5 plugin **ThirdParty module system**. The plugin shall build cleanly on a fresh UE5 installation with the appropriate vcpkg or pre-built binary dependencies. |
| NFR-11 | **Disabled Overhead** | When Enable SPM Overlay is false, per-tick CPU overhead attributable to the SPA component shall be **[TBD]**. Preliminary target: < 0.05 ms/tick (effectively unmeasurable). |
| NFR-12 | **Topology Stability** | Repeated invalidation and regeneration of the same Stroke Layer segment (e.g., the same keyframe edited multiple times) shall produce geometrically identical output and shall not cause progressive topology degradation (accumulating poles, HARPs, or folded faces). |

---

## 4. Requirements Traceability Matrix

Maps each functional requirement group to the Success Criteria (Stage 1) and Use Cases (Stage 2) it satisfies.

| FR Group | Description | Satisfies SC | Satisfies UC |
|----------|-------------|-------------|-------------|
| FR-01 – FR-05 | Animation Data Access | SC-02, SC-03 | UC-01, UC-04, UC-09 |
| FR-06 – FR-13 | Leading-Edge Extrusion | SC-01, SC-08 | UC-04, UC-08, UC-09 |
| FR-14 – FR-19 | Stroke Layer Cache | SC-06 (performance) | UC-04, UC-08 |
| FR-20 – FR-23 | Boolean Union & Post-Processing | SC-01, SC-07, SC-08 | UC-04, UC-08, UC-09 |
| FR-24 – FR-29 | UE5 Component Integration | SC-02, SC-03, SC-04, SC-05 | UC-01, UC-02, UC-03, UC-04, UC-05 |
| FR-30 – FR-33 | Warnings & Diagnostics | SC-10 | UC-06, UC-07 |
| FR-34 – FR-36 | Geometry Backend Interface | SC-09 | UC-10 |
| FR-37 – FR-39 | CLI Reference Tool | SC-11 | UC-09 |
| FR-40 – FR-44 | SPM Display Buffering & Staleness | SC-05, SC-12, SC-13 | UC-04, UC-05 |
| FR-V2-01 – FR-V2-04 | v2.0 Deferred | SC-10 (partial), future | UC-07 (partial), future |

---

## 5. Open Questions for Stage 4

The following items require quantification in Stage 4 (V&V Strategy & Acceptance Criteria) before architecture can be finalized:

| # | Question | Affects |
|---|---------|---------|
| OQ-01 | What is the maximum acceptable SPM update latency (NFR-03)? What does "feels responsive" mean in seconds for an animator workflow? | NFR-03, Stage 4 acceptance criteria |
| OQ-02 | What is the maximum acceptable Stroke Layer cache memory footprint (NFR-04)? | NFR-04, Stage 8 architecture decisions |
| OQ-03 | What is the maximum acceptable per-tick overhead when the overlay is disabled (NFR-11)? | NFR-11, Stage 4 acceptance criteria |
| OQ-04 | What is the default epsilon threshold for leading-edge classification (FR-07)? Needs empirical calibration in Stage 11. | FR-07, FR-11 |
| OQ-05 | What is the decimation/remesh cadence (FR-23)? Every N intervals? When triangle count exceeds a threshold? | FR-23, NFR-04 |
| OQ-06 | Does UE5 expose a per-keyframe-change notification API usable in Editor mode without PIE? (Carries forward R-07 from Stage 2) | FR-16, FR-17, FR-27 |
| OQ-07 | Does UE5's animation evaluation API support reading vertex positions at arbitrary time positions efficiently enough to drive per-vertex displacement computation (FR-06)? | FR-06, NFR-03 |
| OQ-08 | What does "system resource utilization threshold" mean concretely (FR-31)? CPU %, memory %, or a combined heuristic? | FR-31, FR-V2-01 |
| OQ-09 | What are the default and acceptable range for the stale hue (FR-43) and normal SPM color (FR-44)? Should the tint intensity be a separate configurable parameter from the color? | FR-43, FR-44 |

---

## 6. Risks Identified at This Stage

| ID | Risk | Likelihood | Impact | Mitigation |
|----|------|-----------|--------|------------|
| R-09 | UE5 does not expose a suitable per-keyframe-change notification in Editor mode (OQ-06) | Medium | High | Investigate `FEditorDelegates`, Sequencer listeners, and `UAnimSequence` modification callbacks in Stage 5; design a polling fallback if push notification is unavailable |
| R-10 | UE5 animation evaluation API does not support efficient per-vertex world-space position queries at arbitrary times, making leading-edge displacement computation (FR-06) too slow | Medium | High | Profile `UAnimSequence::EvaluateAnimation()` and related APIs in Stage 11 spike; consider caching evaluated poses rather than re-evaluating per vertex |
| R-11 | Post-step topology simplification (FR-13) and post-union cleanup (FR-22) introduce latency spikes that violate NFR-03 for complex or rapidly-changing motion | Medium | Medium | Benchmark cleanup cost separately from union cost in Stage 11; make simplification asynchronous or cadenced if needed |
| R-12 | Parallel Layer-3 segment generation (FR-21) requires CGAL to be thread-safe under concurrent use; CGAL's thread-safety guarantees are partial and depend on kernel and component | High | Medium | Confirm CGAL concurrency tags and thread-safe paths for the operations used; use separate CGAL instances per thread if necessary |

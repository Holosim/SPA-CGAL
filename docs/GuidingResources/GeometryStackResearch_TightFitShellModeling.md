# Best C++ Geometry Stack for Swept-Path Tight-Fit Shell Modeling

## Executive summary

A “tight-fit shell” for a moving 3D object is the boundary of the **swept volume**, meaning the **union of the solid across all poses** in a time interval. This is a well-studied but practically difficult problem because exact envelope construction is analytically and topologically complex, while robust Boolean unions across many time samples are numerically fragile and can be slow. citeturn0search3turn4search27turn0search31turn4search1

The highest-leverage conclusion is that there is rarely a single C++ library that is “best” in isolation. For production quality, you typically combine a **CAD B-Rep kernel** (for NURBS, tolerances, STEP, and CAD-grade topology) with a **mesh or exact-geometry computational geometry library** (for robust unions, meshing, validation, and acceleration structures). citeturn9search5turn0search0turn1search6turn3search2

### Top recommendations

**Open-source, CAD-centric recommendation: Open CASCADE Technology as the primary modeling kernel**  
Choose this when you need NURBS-based CAD workflows (STEP in, B-Rep modeling operations, sweeps along spines, revolve, Boolean fuse, healing, tolerances), and you can accept careful engineering around robustness, tolerances, and partial thread safety. It is explicitly positioned as a C++ platform for 3D surface and solid modeling plus CAD data exchange. citeturn9search5turn3search2turn12search2turn0search5turn1search1turn10search26  
Key reasons: native modeling operators for sweep and revolve. citeturn12search1turn12search0turn12search2, explicit tolerance concepts in topology and precision utilities. citeturn10search0turn10search1turn11view0, official options for parallel processing and “fuzzy” (additional tolerance) in booleans plus OBB acceleration. citeturn12search3turn11view0, LGPL 2.1 with exception. citeturn1search1turn1search5turn1search17

**Open-source, robustness-centric recommendation: CGAL as the Boolean and meshing back-end for a sampled-sweep pipeline**  
Choose this when your swept shell can be represented as a triangle mesh (or you can tessellate a B-Rep). Its polygon mesh Boolean pipeline explicitly recommends exact predicates and exact constructions to avoid topological correctness or self-intersection issues during consecutive Booleans. citeturn0search0turn0search16turn0search20  
Key reasons: strong robustness knobs via kernels (exact predicates and constructions) for corefinement-based mesh booleans. citeturn0search0turn0search35, fully general exact Boolean closure via Nef polyhedra if you can afford heavy exact computation. citeturn1search6turn1search18, explicit support for concurrency tags and evolving thread-safety work. citeturn5search0turn5search4turn0search28turn5search5  
Primary caveat: it is not a CAD kernel and does not provide STEP as a first-class import. citeturn8search6

**Commercial “best-in-class kernel” recommendation: Parasolid if you need industrial-grade tolerance, robustness, and scaling**  
Choose this when you are building a professional CAD-adjacent tool and can pay for licensing. Parasolid is positioned as a geometric modeling kernel with a large modeling API surface and includes convergent modeling (B-Rep and facets). citeturn8search12turn0search2 It also documents multi-processor use in areas including booleans, and it explicitly supports tolerant modeling concepts in its ecosystem documentation. citeturn0search14turn10search21turn10search2

A pragmatic “best stack” for a swept-volume tool that must be both robust and fast is commonly:
- CAD kernel for representation and IO, plus modeling tolerances and healing. citeturn9search5turn3search2turn10search26turn1search1  
- Mesh Boolean and meshing kernel for repeated unions, incremental remeshing, and determinism. citeturn0search0turn1search6  
- Optional volumetric back-end for near-real-time unions at controlled resolution using sparse level sets and threaded CSG operations. citeturn5search23turn6search1turn6search9

## Evaluation criteria for a swept-volume tight-fit shell tool

A swept-volume tool stresses geometry libraries in ways that differ from typical modeling operations like “sweep a profile along a spine.” The workload is dominated by repeated spatial unions of near-coplanar, near-coincident geometry across time, plus envelope self-intersections and tolerance accumulation. citeturn4search15turn4search27turn11view0

Key attributes to evaluate, and why they matter for swept volumes:

Robust support for sweep, revolve, and NURBS matters primarily for IO and “path definition,” not because it directly solves the swept-union problem. For example, a CAD kernel’s sweep operators create a swept solid from a profile and spine. That is related, but the swept volume of an arbitrary moving solid is still a set union over poses, or an envelope extraction problem. citeturn12search2turn12search1turn4search27turn0search31

Robust Boolean operations must tolerate near-degenerate intersections and cumulative error. In mesh booleans, CGAL explicitly warns that exact predicates alone can fix topology but still yield self-intersections in the embedding unless constructions are also exact, which becomes critical under consecutive unions. citeturn0search0turn0search20 In B-Rep booleans, Open CASCADE’s Boolean component documentation explicitly discusses self-interferences due to tolerances and exposes “additional tolerance” and parallel processing options. citeturn11view0turn12search3

Exact versus approximate geometry is not a philosophical choice here. Exact envelope computation exists in the research literature and can represent boundaries via ruled and developable patches, but implementation complexity is high and self-intersections require sophisticated trimming and arrangement logic. citeturn4search15turn4search27turn4search1turn4search5 Approximate approaches (sampling plus unions, or volumetric level sets) are easier to ship and can be made conservative or error-bounded via adaptive stepping, but they trade accuracy for throughput. citeturn0search3turn4search3turn5search23turn6search1

Tolerance handling is central because CAD kernels are typically “tolerant modelers,” with explicit per-shape tolerances and precision utilities. Open CASCADE exposes tolerance queries for vertices, edges, and faces and provides general precision utilities and shape healing. citeturn10search0turn10search1turn10search26 ACIS documents tolerant modeling using global tolerances like SPAresabs and per-feature tolerances. citeturn10search3turn10search7turn10search11 Parasolid documents session and local precision concepts used for tolerant modeling workflows. citeturn10search2turn10search21

Performance and real-time suitability hinges on broad-phase pruning, acceleration structures, batching unions, and possibly switching representations. CGAL provides AABB trees for intersection and distance queries. citeturn5search1turn5search21 Embree provides high-performance ray queries and is designed for integration into applications that need fast spatial queries. citeturn9search10turn2search7 OpenVDB provides a sparse hierarchical volumetric structure and threaded CSG operations that are attractive when you need stable results at interactive rates. citeturn5search23turn6search1turn6search9

Multithreading and GPU support should be evaluated as “where can I safely parallelize.” CGAL has explicit concurrency tags and has ongoing work on thread safety in key components. citeturn5search0turn5search4turn0search28turn5search5 Open CASCADE exposes parallel flags in Boolean APIs and has parallelization hooks, but community guidance still warns that full thread safety is not guaranteed in many contexts. citeturn12search3turn5search2 For GPU, NVIDIA’s OptiX is a ray tracing framework for GPU acceleration, useful for spatial queries or voxelization pipelines, not for exact booleans. citeturn6search2turn6search10

Licensing matters because kernels may be dual-licensed or proprietary. CGAL is dual-licensed with parts under LGPL and parts under GPL, with commercial licenses available. citeturn1search0turn1search8 Open CASCADE is LGPL 2.1 with an exception. citeturn1search1turn1search5 OpenVDB’s licensing requires care because the upstream project states it has relicensed to Apache 2.0, while some downstream documentation still references the earlier MPL 2.0. citeturn7view0turn6search0turn9search39

## Library landscape and comparison

The candidates naturally fall into three functional buckets:

CAD B-Rep kernels for NURBS, topology, STEP, and tolerant modeling, a computational geometry library for robust mesh operations and exact arithmetic, and supporting libraries for visualization, acceleration, and volumetric representations. citeturn9search5turn0search0turn5search23turn2search1

### Comparison table

The table below is biased toward building a swept-volume tool that can import CAD, compute swept unions robustly, and export a shell suitable for downstream use.

| Candidate | Best role in swept-volume tool | Sweep / revolve / NURBS support | Boolean robustness for repeated unions | Exactness model | Tolerances | Parallel / GPU notes | IO and ecosystem | License and platforms | Notes and primary sources |
|---|---|---|---|---|---|---|---|---|---|
| Open CASCADE Technology | Primary CAD kernel, B-Rep modeling, CAD IO, healing, optional boolean-based sweep union | Revolve and prism sweeps in BRepPrimAPI, complex sweeps in BRepOffsetAPI. NURBS via BSpline classes and “NURBS convert” | General Fuse and Boolean components with options like parallel processing, additional tolerance, OBB. Robustness depends on input validity and tolerance management | Floating arithmetic with explicit precision utilities | Per-subshape tolerances and precision utilities, shape healing toolkit | Parallel boolean flags exposed, but full thread safety is not universally guaranteed in practice | STEP reader translates STEP entities to shapes, includes tutorials and tooling | LGPL 2.1 with exception. Officially certified on major desktop and some mobile platforms | OCCT positioned as C++ platform for modeling and exchange. citeturn9search5turn12search1turn12search0turn12search2turn8search1turn8search11turn12search3turn11view0turn3search2turn1search1turn9search9turn10search26turn5search2 |
| CGAL | Robust mesh booleans, exact computation path, meshing and validation back-end | Not a CAD NURBS kernel, mostly polyhedral and mesh representations | Corefinement-based mesh booleans. Documentation explicitly recommends exact predicates and exact constructions for consecutive operations. Nef polyhedra are closed under boolean set operations | Exact arithmetic available via kernels. Nef polyhedra support full set operations in generality | Tolerance is typically replaced by exact predicates or controlled numeric kernels | Concurrency tags exist, and some kernels have become thread-safe. Parallel variants exist in some components | No STEP support as first-class. Strong examples and demos | Dual GPL/LGPL with commercial licensing options. Cross-platform C++ | Mesh booleans and Nef polyhedra are the key differentiators for robustness. citeturn0search0turn0search20turn1search6turn1search18turn5search0turn5search4turn0search28turn5search5turn8search6turn1search0turn1search8 |
| Parasolid | Commercial kernel option for CAD-grade booleans, tolerant modeling, industrial robustness | Broad modeling techniques including free-form surface and sheet modeling, convergent modeling mentioned | Industrial-grade boolean and modeling reliability. Multi-processor use documented in areas including booleans | Exact B-Rep modeler per Parasolid documentation | Tolerant modeling and local precision concepts are documented | Multi-processor support discussed by vendor community resources | Often paired with commercial IO stacks, prototyping environment and documentation emphasized | Proprietary. Platform coverage depends on vendor distribution | Use when you need the highest reliability and can license. citeturn8search12turn0search2turn0search14turn10search2turn10search21turn8search16 |
| ACIS | Commercial kernel option similar to Parasolid with tolerant modeling, multi-thread guidance | ACIS positioned for industrial-grade modeling workflows | Strong boolean and healing capabilities as a kernel, typical of CAD modelers | Exact model with tolerant modeling features documented | Tolerant modeling and tolerance variables documented | Marketing and docs emphasize multi-threaded APIs and multi-core support | Integrates with commercial ecosystems, documentation often gated | Proprietary. Platform depends on vendor distribution | Use if your org already licenses ACIS or needs its ecosystem. citeturn3search0turn3search13turn10search3turn10search11turn3search1 |
| VTK | Visualization plus “good enough” mesh booleans for clean data, not CAD robustness | Not a NURBS modeling kernel | Boolean filter exists but warns about unexpected results with non-manifold inputs and needs clean data | Floating mesh operations | No CAD tolerance model, mesh-centric cleanup required | Threading depends on pipeline. Mainly CPU | Strong C++ visualization ecosystem, many examples | BSD 3-clause | Useful for debugging and visualization, not the core robust union engine. citeturn2search0turn2search4turn2search1 |
| OpenVDB | Volumetric swept union for near-real-time, robust topology at chosen resolution | Not a NURBS kernel, works on sparse voxel grids and level sets | Threaded CSG union, intersection, difference in tooling. Great for many-pose unions | Approximate, resolution-dependent | Implicit tolerance through voxel size and narrow-band settings | Threaded operations documented. GPU path exists via NanoVDB in ecosystem release notes | Cookbook and tooling. “About” emphasizes sparse volumetric data | Upstream states relicensed to Apache 2.0. Some downstream still references MPL 2.0, so confirm version | Best practical fallback when mesh booleans become too slow or fragile. citeturn5search23turn6search1turn6search9turn7view0turn9search23turn6search0turn9search39 |
| Eigen | Math and transforms, not geometry kernel | N/A | N/A | Floating linear algebra | N/A | Vectorized CPU math. User manages threading | Standard dependency in C++ geometry stacks | MPL 2.0 | Use for kinematics, transforms, sampling, numerical utilities. citeturn2search2turn2search6 |
| Embree | Accelerate spatial queries for sampling, validation, and voxelization | N/A | N/A | Ray queries on triangle geometry | N/A | Highly optimized CPU ray tracing kernels, supports multiple platforms and some GPU targets for ray tracing workloads | Integrates as spatial query engine | Apache 2.0, multi-platform, vendor docs emphasize performance | Use for fast intersection tests or occupancy sampling on meshes. citeturn9search10turn2search7turn9search2 |
| STEPcode | STEP parsing and schema tooling, not modeling kernel | N/A | N/A | N/A | N/A | N/A | ISO 10303 focused C++ libraries and tools | Open-source project, cross-platform goals | Use if you need low-level STEP processing beyond CAD kernels, or schema-driven pipelines. citeturn3search3turn3search7 |
| OptiX | GPU ray tracing framework for acceleration, not modeling kernel | N/A | N/A | Ray tracing queries | N/A | GPU acceleration for ray tracing via CUDA-centric API | SDK and documentation for GPU ray tracing | Proprietary but commonly usable commercially, confirm SDK license version | Use to accelerate voxelization or sampling, not for exact booleans. citeturn6search2turn6search10turn6search18 |

A practical reading of this table is that Open CASCADE Technology is the strongest single open-source “center of gravity” if you need CAD-type data and NURBS. CGAL is the strongest open-source robustness engine when you are willing to operate on meshes or exact polyhedral sets. Parasolid is the strongest option when you need CAD-grade kernel behavior without taking on the risk of open-source kernel edge cases. citeturn9search5turn0search0turn0search2turn1search1turn1search0

## Swept-volume tight-fit shell algorithms and implementation notes

Swept volume computation has two broad families: envelope-based methods that aim at an exact boundary representation, and union-based methods that approximate the union of poses, either in boundary form or in a volumetric implicit representation. citeturn4search15turn4search27turn0search31turn5search23

image_group{"layout":"carousel","aspect_ratio":"16:9","query":["swept volume of moving solid illustration","rigid body swept volume envelope ruled surface developable surface","Minkowski sum path sweep illustration","signed distance field level set CSG union illustration"],"num_per_query":1}

### Algorithm options that map well to real C++ implementations

Envelope extraction for exact swept boundaries  
Classic results show that swept volume boundaries can be composed of ruled and developable surface primitives for polyhedral models, with the final outer boundary being a subset of these primitives, and that computing it robustly often involves envelope theory and trimming after self-intersections. citeturn4search15turn0search3turn4search27turn4search5  
Practical implication: implementing this exactly is a multi-month to multi-year effort if you want full generality of rigid motions, because you need accurate characteristic curve tracking, robust surface-surface intersection, arrangement and trimming. citeturn4search1turn4search27turn4search5  
Where libraries fit: a CAD kernel can provide surface representations, intersection tools, and trimming infrastructure, but you still have to implement envelope computation logic on top. citeturn11view0turn12search17turn12search2

Sampling plus robust Boolean union of poses  
The most common engineering approach is to sample time, transform the solid at each sample, and compute the union of the resulting shapes. The swept volume definition as a union of poses is standard. citeturn0search31  
The core challenge is robustness and performance under many unions. For a mesh-based route, CGAL’s documentation is explicit that consecutive Boolean operations should be performed with exact predicates and exact constructions when possible. citeturn0search0turn0search20  
For a B-Rep-based route, Open CASCADE offers Boolean fusing and a Boolean operations component with options for additional tolerance and parallel processing. citeturn0search5turn12search3turn11view0  
Where libraries fit: Open CASCADE Technology for IO, tessellation, and possibly B-Rep fusing, with CGAL for stable mesh unions and post-processing. citeturn3search2turn0search0turn8search6turn10search0

Sampling plus Minkowski-sum style constructions in special cases  
For pure translations, swept volumes connect to Minkowski sums, and the literature treats Minkowski sums as a special case framing for sweeps. citeturn0search31turn4search31  
For practical implementation, GPU or volumetric approaches that build a signed distance field and then extract an iso-surface can approximate Minkowski sums and unions more reliably than exact boundary operations under complex intersections. citeturn6search31turn5search23turn6search1  
Where libraries fit: OpenVDB for sparse SDF union and fast topology-stable results, optionally GPU voxelization to accelerate sampling. citeturn5search23turn6search1turn6search39turn6search2

Swept-surface generation plus union  
Kim, Varadhan, Lin, and Manocha describe a pipeline that enumerates a superset of swept boundary primitives, including ruled and developable surfaces, then reconstructs the outer boundary through sampling and reconstruction. citeturn0search3turn4search15turn0search15  
Practical implication: even “approximation papers” are considerably more complex than “sample and union,” but they can yield tighter boundaries per sample count, especially for high-speed motion where naive sampling would need many steps. citeturn0search3turn4search3

### Meshing strategies for union-based swept shells

B-Rep to mesh conversion should be deterministic and tolerance-aware  
If you import STEP into a B-Rep kernel, you will typically tessellate to a triangle mesh for mesh booleans. Open CASCADE exposes a meshing pipeline via classes like BRepMesh_IncrementalMesh and also supports extracting triangulations from faces. citeturn5search38turn10search0  
For repeatability, fix meshing parameters, and cache triangulations, then invalidate only when needed. Open CASCADE provides functions to clean cached polygonal representations. citeturn10search4

Corefined mesh booleans require watertight, intersection-managed input  
CGAL’s mesh Boolean approach uses corefinement as an internal building block and documents the importance of exact kernels in consecutive operations. citeturn0search0turn0search20  
Practical technique: apply mesh cleanup and ensure manifoldness before entering the union loop, then periodically remesh or simplify to control triangle count growth.

Volumetric route requires choosing a resolution that quantifies error  
With level sets, your primary error dial is voxel size and narrow-band extent. OpenVDB’s core value proposition is efficient manipulation of sparse volumetric grids, and it provides threaded CSG operations that are natural for repeated unions. citeturn5search23turn6search1turn6search9  
Practical technique: compute a conservative bounding box for all poses, allocate the sparse grid accordingly, then apply union operations incrementally. Extract the final shell with iso-surface extraction downstream.

### Handling self-intersections and degeneracies

Envelope self-intersections are fundamental  
Envelope-based swept surfaces can self-intersect and require trimming to yield the true outer boundary. The literature explicitly frames these as limitations and key difficulties of swept-volume boundary generation. citeturn4search27turn4search1turn4search5

Boolean unions amplify tolerance and near-coincidence issues  
Open CASCADE’s Boolean documentation explicitly gives examples of shapes that are “valid” but self-interfered due to tolerances and therefore unacceptable as Boolean arguments. citeturn11view0  
This matters for swept unions because successive poses often differ by small transforms, creating near-coincident faces and edges.

Robustness knobs that usually matter most in practice  
For mesh booleans, use exact predicates and exact constructions when feasible, especially for consecutive operations. citeturn0search0turn0search20  
For B-Rep booleans, use fuzzy tolerance settings intentionally and validate inputs for self-interference. Open CASCADE exposes SetFuzzyValue for additional tolerance and supports options like OBB. citeturn12search3turn11view0  
For tolerant kernels, explicitly manage precision or local tolerances to avoid importing low-quality data that cascades into failures, which is a major theme in both Parasolid and ACIS documentation. citeturn10search2turn10search3turn10search21

### Performance optimizations that are usually decisive

Adaptive time sampling using conservative advancement concepts  
Instead of fixed dt, compute dt based on motion magnitude and local geometric features. Continuous collision detection literature by Tang, Kim, and Manocha targets interactive CCD using conservative advancement variations, which can be repurposed conceptually to adapt sampling density where motion or proximity to the existing union boundary is “fast-changing.” citeturn4search3

Spatial partitioning and incremental unions  
Batch poses into chunks, union inside each chunk, then union chunk results in a tree reduction. This reduces worst-case growth in intermediate complexity and enables parallel execution, especially in mesh workflows. CGAL explicitly has concurrency tags in parts of the library, and Open CASCADE exposes parallel flags for Boolean processing. citeturn5search0turn12search3turn11view0

Acceleration structures for broad-phase pruning and validation  
Use AABB trees to test whether a new pose instance even intersects the current accumulated union. CGAL provides AABB tree structures for efficient intersection and distance queries. citeturn5search1turn5search21  
For ray-based occupancy tests or voxelization steps, Embree is commonly used as a high-performance ray query engine. citeturn9search10turn2search7

GPU acceleration where it actually helps  
GPU helps most in voxelization, distance field construction, and large-scale sampling, not in exact boundary booleans. NVIDIA’s OptiX is a CUDA-centric ray tracing framework that can accelerate ray queries and related rasterization style workloads feeding a volumetric pipeline. citeturn6search2turn6search10turn6search6

## Prototype architecture and C++ integration plan

A robust prototype architecture should explicitly separate representation, motion sampling, union strategy, and verification. The architecture below is “hybrid by design” so you can swap union back-ends without changing the motion logic.

```mermaid
flowchart TD
  A[CAD input: STEP/IGES or mesh] --> B[B-Rep kernel import and healing]
  B --> C[Tessellation or implicit conversion]
  A --> D[Motion definition: time-parameterized SE(3)]
  D --> E[Adaptive sampler]
  E --> F[Pose instances T(t_i)]
  F --> G1[Mesh union backend]
  F --> G2[Volumetric union backend]
  C --> G1
  C --> G2
  G1 --> H[Post-process mesh: remesh, decimate, repair]
  G2 --> I[Extract iso-surface mesh]
  I --> H
  H --> J[Validation and regression tests]
  J --> K[Export: mesh or downstream CAD]
```

### Concrete C++ integration plan

CAD import and canonicalization layer  
Use Open CASCADE Technology to import STEP into TopoDS_Shape via STEPControl_Reader, and apply healing as needed so downstream booleans are less likely to fail. citeturn3search2turn10search26  
If you need to normalize surface types, you can convert geometry to NURBS with BRepBuilderAPI_NurbsConvert, which explicitly converts curves and surfaces supporting edges and faces into BSpline representations. citeturn8search11turn8search1  
This step is crucial if you rely on consistent tessellation properties or want reproducibility across imported data. citeturn10search26turn10search33

Representation conversion  
Option A is mesh union, meaning you tessellate the B-Rep and operate on a triangle mesh. Open CASCADE exposes face triangulations and meshing tools, and provides explicit triangulation access in BRep_Tool. citeturn10search0turn5search38  
Option B is volumetric union, meaning you convert each pose to a level set and union those level sets. OpenVDB is purpose-built for sparse volumetric data, and its tools include threaded CSG union copy operations. citeturn5search23turn6search1turn6search9

Motion and sampling  
Represent motion as a time-parameterized rigid transform in SE(3). Use Eigen for robust, well-tested linear algebra and transforms. citeturn2search2  
Implement adaptive sampling driven by maximum linear displacement of bounding volume features and angular velocity bounds, and optionally by testing pose-to-union “novelty” through broad-phase AABB checks. CGAL’s AABB tree can support these queries in a mesh-based pipeline. citeturn5search1turn5search21

Union back-end selection  
Mesh union back-end: use CGAL Polygon Mesh Processing corefinement and boolean union operations, and pick an exact kernel strategy suitable for consecutive operations, consistent with CGAL’s own guidance. citeturn0search0turn0search20turn0search16  
Exact-polyhedral back-end for maximum correctness: if your input can be expressed as Nef polyhedra and performance is acceptable, Nef_polyhedron_3 is closed under boolean set operations and can represent non-manifold configurations. citeturn1search6turn1search18  
B-Rep union back-end: for certain pipelines, you may choose to fuse TopoDS_Shapes directly and use boolean options like SetFuzzyValue and SetRunParallel. This is typically more CAD-native but requires careful tolerance and validity management. citeturn0search5turn12search3turn11view0  
Volumetric back-end: for near-real-time or very large sample counts, iterate OpenVDB level set unions and extract a mesh at the end. Threaded CSG operations are documented in OpenVDB tooling. citeturn6search1turn6search9turn5search23

Validation and correctness testing  
At minimum, validate that results are closed and consistent, and regression test on known motions. Open CASCADE’s Boolean documentation emphasizes that even “valid” shapes can be self-interfered due to tolerances. Your tests should include near-coincident and tolerance-stressed cases. citeturn11view0  
For mesh pipelines, run intersection detection and manifoldness checks, and compare a mesh-based union against a coarser volumetric union as a sanity oracle. CGAL’s exact predicate story for intersection detection is explicitly tied to point types from exact predicate kernels. citeturn0search35turn0search0

A useful testing strategy is to treat volumetric results not as a final output but as an independent reference. This is often more stable in degenerate situations, at the cost of resolution error. citeturn5search23turn6search1

## Development effort and performance expectations

Assumptions are necessary to give meaningful estimates because both object complexity and targeted sampling frequency were unspecified. The estimates below assume:
- Input geometry after tessellation is on the order of 50k triangles for a single pose, or a B-Rep with tens to low hundreds of faces.
- Typical motion duration is seconds to minutes.
- Target “reasonable” sample rates are 30 to 240 poses per second for near-real-time preview, and lower rates with adaptive sampling for offline quality.  
These assumptions are consistent with typical interactive geometry workloads and the fact that the sampled union approach scales roughly linearly with sample count and nonlinearly with intermediate mesh complexity growth.

### Effort estimates by approach

Mesh-based sampled union, Open CASCADE import + CGAL booleans  
A first working prototype with STEP import, deterministic tessellation, adaptive sampling, and CGAL mesh union is usually feasible in roughly 6 to 12 weeks for an experienced C++ engineer, assuming you constrain inputs to watertight solids and accept occasional failure cases that you can detect and fall back from. The time is dominated by interoperability, mesh conditioning, and failure handling, not by calling the union API. citeturn3search2turn0search0turn0search20turn8search6  
A production-quality version that handles messy CAD, near-tangencies, and long sequences robustly can take significantly longer due to geometry conditioning, tolerance normalization, and performance engineering. Open CASCADE’s own documentation illustrates how tolerances can create self-interference and invalidate otherwise “valid” inputs, which is exactly the kind of edge case you must engineer around for repeated unions. citeturn11view0turn10search1

Volumetric union with OpenVDB  
A prototype that voxelizes each pose and unions level sets is often faster to get stable results from, because CSG on level sets is less sensitive to triangle degeneracies, and OpenVDB provides threaded CSG operations as first-class tools. citeturn6search1turn6search9turn5search23  
Expect 4 to 8 weeks for a usable prototype if you already have a voxelization and SDF conversion step, longer if you build a high-quality narrow-band SDF pipeline from scratch. OpenVDB includes a cookbook and detailed API docs, which lowers integration risk. citeturn6search21turn5search23  
The main engineering risk is picking voxel resolution and ensuring watertightness of conversions, as these define the approximation error.

Exact envelope extraction  
Implementing an envelope-based method comparable to the swept-volume envelope literature is a large effort, because it requires characteristic curve computation, robust trimming of self intersections, and a representation robust to degeneracies. The survey and envelope literature emphasize both the richness of the boundary structure and the limitations of existing algorithms. citeturn4search1turn4search27turn4search5  
This is typically a research project in its own right unless your motions are highly restricted.

### Performance expectations

Mesh sampled union  
If you attempt to union 50k-triangle meshes at 60 Hz naively, even high-quality mesh boolean libraries will struggle for true real-time on a single core because each union is computationally heavy and intermediate mesh complexity grows. CGAL’s documentation focus is correctness and robustness, not real-time throughput, and it explicitly encourages exact constructions for consecutive operations, which increases computational cost. citeturn0search0turn0search20  
Near-real-time is more realistic if you:
- reduce sampling via adaptive dt,  
- use broad-phase pruning so many poses do not trigger a full union,  
- batch unions in parallel tree reductions,  
- periodically simplify the accumulated mesh.  
CGAL’s concurrency tags and Open CASCADE’s parallel boolean flags support the idea of parallelizing parts of the pipeline. citeturn5search0turn12search3

Volumetric union  
Near-real-time is more achievable with sparse level sets, because each pose update is a grid merge, and OpenVDB documents threaded CSG operations. citeturn6search1turn6search9turn5search23  
For interactive previews, the common expectation is that voxel size is chosen so the shell is “visually tight” rather than mathematically exact, then a higher-resolution pass is run offline for final output.

GPU-assisted sampling  
GPU ray tracing frameworks like OptiX can accelerate ray-based occupancy checks or assist voxelization pipelines, but they do not replace the need for a modeling kernel to produce a CAD-quality boundary representation. citeturn6search2turn6search10  
Embree offers a CPU-centric alternative when you need very fast spatial queries over meshes and do not want GPU dependency. citeturn9search10turn2search7

## Prioritized references and official documentation

The list below prioritizes official documentation and primary papers. Each citation is a direct link to the referenced document.

Open CASCADE Technology modeling, booleans, tolerances, and IO  
Open CASCADE Technology positioning as C++ platform for modeling and CAD exchange. citeturn9search5  
Official licensing statement, LGPL 2.1 with exception. citeturn1search1turn1search5  
Boolean Operations component specification, including tolerance-driven self-interference examples and options like parallel processing and additional tolerance. citeturn11view0  
Boolean API class with SetRunParallel, SetFuzzyValue, and OBB controls. citeturn12search3  
Sweep and revolve operators: MakeSweep overview, MakeRevol and pipe shell sweep classes. citeturn12search1turn12search0turn12search2  
NURBS and BSpline surface classes and full shape conversion to NURBS. citeturn8search1turn8search11  
STEP import: STEPControl_Reader. citeturn3search2  
Precision and tolerance utilities and per-subshape tolerance access. citeturn10search1turn10search0  
Shape healing toolkit overview. citeturn10search26  
Build and supported platform documentation. citeturn9search1turn9search9

CGAL robustness, booleans, exactness, and concurrency  
Polygon Mesh Processing manual, including explicit recommendations about exact predicates and exact constructions for consecutive booleans. citeturn0search0turn0search20  
Example code for corefinement union. citeturn0search16  
Nef polyhedra documentation showing closure under boolean set operations and capability for non-manifold geometry. citeturn1search6turn1search18  
CGAL concurrency tags. citeturn5search0turn5search4  
CGAL licensing rationale and dual license model. citeturn1search0turn1search8  
Evidence of missing STEP support in CGAL workflows. citeturn8search6

Swept volume and envelope algorithms, primary papers  
Kim, Varadhan, Lin, Manocha, “Fast Swept Volume Approximation of Complex Polyhedral Models,” paper and project materials. citeturn0search3turn0search15turn4search15  
Abrams et al., “Computing Swept Volumes,” survey style paper. citeturn4search1  
Rossignac et al., “Boundary of the volume swept by a free-form solid in screw motion,” and associated paper. citeturn4search27turn0search23  
Wallner and Yang, “Swept volumes of many poses,” definition and Minkowski sum context. citeturn0search31  
Rabl et al., envelope computation for moving surfaces. citeturn4search5  
Tang, Kim, Manocha, “C2A: Controlled Conservative Advancement,” continuous collision detection for interactive applications, useful as conceptual basis for adaptive sampling. citeturn4search3

Parasolid and ACIS tolerance and parallelism documentation  
Parasolid product overview and feature positioning. citeturn0search2turn8search12  
Parasolid multi-processor discussion including booleans. citeturn0search14  
Parasolid tolerant modeling and local precision documentation pages. citeturn10search2turn10search21  
ACIS modeling positioning, multi-thread emphasis, and tolerance documentation for tolerant modeling. citeturn3search13turn10search3turn10search11

Volumetric and acceleration libraries  
OpenVDB about page, sparse volumetric structure positioning. citeturn5search23  
OpenVDB threaded CSG union and related tools API. citeturn6search1turn6search9  
OpenVDB relicensing note in upstream repository and release information. citeturn7view0turn9search23  
Embree overview and release under Apache 2.0, platform support, and integration focus. citeturn9search10turn2search7turn9search2  
OptiX ray tracing documentation and SDK references. citeturn6search2turn6search18turn6search10  
Eigen official licensing and positioning as a C++ linear algebra library. citeturn2search2turn2search6

Visualization toolchain  
VTK license and boolean filter documentation with notes about non-manifold inputs and “clean data.” citeturn2search1turn2search0turn2search4
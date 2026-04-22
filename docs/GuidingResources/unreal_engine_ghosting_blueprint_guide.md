# Ghosting in Unreal Engine with PCG

This guide walks through creating a "ghosting" effect in Unreal Engine—replicating multiple instances of a moving or animated mesh that represent frames in the past and future. The approach uses **Procedural Content Generation (PCG)** to ensure good performance.

---

## Overview

A typical ghosting or trailing effect duplicates the mesh at previous and future positions. Your character or animated mesh appears with multiple "ghosts" at each moment in time, highlighting its path. This is especially useful in debugging or visualizing physics or complex animations.

**Requirements**:
1. Work with any animation method: embedded keyframes, skeletal animation, physics simulation, etc.
2. Adjust the number of frames (or time steps) to display before and after the current time.
3. Employ Unreal Engine's PCG for runtime performance.
4. Expose variables in the Blueprint to assign unique materials to highlight frames in the past or future.
5. Expose a variable for a collision-warning material that replaces the normal ghost material whenever a ghost overlaps or collides with other geometry.

---

## Step-by-Step Implementation

### 1. Create a Blueprint Actor

1. **Name**: For example, `BP_GhostingActor`.
2. **Add Components**:
   - **Scene Root**: The default root.
   - **Skeletal Mesh or Static Mesh Component**: This mesh is your main animated object. It could be a character or any object with an animation track.
   - **PCG Component**: This allows you to drive your ghost instances via a PCG graph.

### 2. Set Up Variables

Open the **Blueprint** (in the **Event Graph** or **Class Defaults**) and create these variables (make them **public** so they're visible in the **Details Panel**):

1. **`NumberOfPreviousFrames`** (`int`): How many frames in the past to show.
2. **`NumberOfFutureFrames`** (`int`): How many frames in the future to show.
3. **`GhostMaterial_Past`** (`MaterialInterface`): Material used for past frames.
4. **`GhostMaterial_Future`** (`MaterialInterface`): Material used for future frames.
5. **`GhostMaterial_CollisionWarning`** (`MaterialInterface`): Material used when a ghost overlaps or collides with the environment.
6. **`TimeStep`** (`float`): Time interval between each ghost frame. This might default to your project’s frame rate (e.g., `1/30` or `1/60`), or you could allow adjusting for a slower/faster ghosting.

### 3. Tracking the Mesh Transform over Time

The ghosting effect requires storing or predicting the transform (position, rotation, scale) of the mesh for both previous and future frames.

#### For Previous Frames

1. **Create an Array**: `PreviousTransforms` of type `Transform`, size = `NumberOfPreviousFrames`.
2. **Each Tick** (or each animation update):
   - Capture the current transform of the mesh.
   - Insert it into a buffer, discarding the oldest transform when the array surpasses `NumberOfPreviousFrames`.

#### For Future Frames

Handling future frames is trickier if you rely on real-time physics or non-deterministic simulation. Options:
1. **Predictive**: If your animation is purely a keyframed or deterministic track, you can query future frames from the Animation/Sequence.
2. **Simulated**: If using physics or dynamic influences, you might do a short simulation in parallel or store transforms as the gameplay unrolls.

**Simple Keyframed Approach**:
1. Create an array `FutureTransforms` of type `Transform`, size = `NumberOfFutureFrames`.
2. Query the animation asset or timeline for transforms at intervals `+TimeStep`, `+2*TimeStep`, etc.

### 4. Using PCG to Generate Instances

**Procedural Content Generation** in UE can help instancing geometry more efficiently:

1. **Create a PCG Graph**: `GhostingPCGGraph`.
2. **Input Node**: Provide a set of transforms for the ghosts.
3. **Spawner Node**: Instantly replicate the geometry at those transforms, using Instanced Static Mesh (ISM) or Hierarchical Instanced Static Mesh (HISM) for performance.
4. **Material Overrides**: You can either supply the material in the spawner node or define it in a subsequent **Set Material** node.

**Integration with the Blueprint**:
1. **Blueprint -> PCG**: Set up references so your `BP_GhostingActor` can feed the `PreviousTransforms` and `FutureTransforms` into the PCG Graph.
2. **Tick or OnUpdate**: As the arrays update, you regenerate the list of transforms in the PCG graph.
3. **One PCG Component**: This resides on `BP_GhostingActor`, referencing `GhostingPCGGraph`.

### 5. Applying Materials for Past/Future and Collision Detection

1. **Define Past/Future**: Inside your PCG graph or blueprint logic, for each transform that belongs to a "past" frame, apply `GhostMaterial_Past`; for a "future" frame, apply `GhostMaterial_Future`.
2. **Collision Check**: If a ghost transform overlaps with other geometry, swap the material to `GhostMaterial_CollisionWarning`.

#### Checking Overlaps

- In your Blueprint, you can do a **SphereTrace** or **BoxTrace** at each ghost transform location.
- Alternatively, you can spawn an ISM/HISM with collision enabled and let Unreal detect overlap events. The PCG system can handle collision, but you must ensure collisions are turned on in the instance settings.
- If an overlap is detected, you can update the instance material to `GhostMaterial_CollisionWarning` for that specific instance.

### 6. Refreshing the Ghost Instances

1. **Event Tick** (or at a lower frequency if performance is a concern):
   - Capture the new current transform.
   - Update `PreviousTransforms` / `FutureTransforms`.
   - Pass them to the PCG Graph.
   - The PCG system regenerates or updates the instance transforms.
2. **Optimization**: Use blueprint logic or a manager to refresh less frequently than every Tick if you prefer. A 30fps refresh for ghosting is often sufficient.

### 7. Displaying or Hiding Ghosts

- You can provide a **`bShowGhosts`** boolean variable in your blueprint to toggle the entire effect.
- If `bShowGhosts == false`, you can skip sending transforms to the PCG graph.

---

## Putting It All Together

1. **Blueprint Setup**:
   - **Variables**: `NumberOfPreviousFrames`, `NumberOfFutureFrames`, `GhostMaterial_Past`, `GhostMaterial_Future`, `GhostMaterial_CollisionWarning`, and `TimeStep`.
   - On every **Tick** (or timed function), record or predict the transforms, storing them in arrays.
2. **PCG Graph**:
   - Takes an array of transforms as an **Input Node**.
   - Uses a **Spawner Node** to place instanced versions of the mesh.
   - Applies the correct material based on whether the transform is from the past or future, or if collision is detected.
3. **Collision Overlaps**:
   - Either do a blueprint trace at each stored transform or enable collisions on the PCG instances.
   - If overlap is detected, switch the instance's material to `GhostMaterial_CollisionWarning`.

With this pipeline, you create a robust ghosting system that performs well even with many duplicates, leveraging PCG's efficient instancing. You can visualize your mesh’s motion, debug collisions, and highlight the past and future positions in real time.

---

## Tips and Best Practices

- **Frame vs. Time**: If your animation is not strictly 1:1 with frames, define a suitable time slice so that your ghosts are evenly spaced.
- **Performance**: Instanced Static Mesh or Hierarchical Instanced Static Mesh are highly efficient for rendering many copies of the same geometry. This is why PCG is crucial.
- **Collisions in PCG**: For best performance, you might rely on a blueprint-based overlap check instead of enabling collisions on hundreds of instanced meshes.
- **Future Data**: If future motion is not deterministic, consider limiting or disabling future ghosts.
- **Animation Notifiers**: If your motion is purely driven by an animation asset, you can integrate with Animation Notifies or the Anim Blueprint to track transforms.

---

**Enjoy your real-time ghosting visualization in Unreal Engine!**


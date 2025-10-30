# Roadmap

Short, focused status and next steps for development.

## Status
- No scene editor yet — focus is on core systems first.
- Some features are temporarily hardcoded (e.g., lighting); these will be refactored.
- `ahi` provides the audio API skeleton only; no backend enabled by default.
- `rhi` is Vulkan-first; OpenGL may be added for testing.

## Working now
- Resource management: buffers, textures, pooled allocations, staging/upload semantics.
- Compute pipeline API: dispatch, binding, and GPU/CPU sync with graphics.
- Material builder + runtime cache for pipelines and descriptor layouts.
- Texture pipeline fixes: formats, uploads, mipmaps, samplers.
- Automatic resource transitions: reduce manual barriers and misuse.
- Transferring existing logic from `master` that has not been added to `dev` after the refactoring (e.g., InputSystem).

## Short-term
- Scene management: entity/component scene graph and simple JSON serialization.
- Rendering pipeline: compose ordered passes (inputs → outputs → dependencies).
- Example (deferred): G-buffer → Lighting → Forward/Transparency → Postprocess → Present
- Multiple cameras: per-camera framebuffers and stacking (main + UI, etc.).
- Creating a well-defined API for audio and an implementation with OpenAL.
 
## Mid-term
- Migrate shader authoring to Slang (single-source → targets).
- Command buffers: pre-record/reuse; strategies tuned per GPU type (discrete vs integrated).
- Use Vulkan sync2, descriptor indexing, dynamic offsets to reduce overhead.
- Enable batched/instanced/indexed draws and consider stencil-based effects.
- A proper UI system (e.g., using Dear ImGui).
- CPU & GPU profiling.
- Proper collision detection, physics, etc.

## Long-term (improbable to achieve soon)
- Editor & tooling when systems are stable.
- Additional RHI backends only if needed.

# Boza Public API

> [!NOTE]
> This page is the map for the public gameplay-facing API exported from `boza/public`. The detailed explanations live in the per-module guides.

## Contents

- [Start Here](#start-here)
- [What `import boza;` gives you](#what-import-boza-gives-you)
- [Module Guides](#module-guides)
- [Choose by Task](#choose-by-task)
- [Repository Examples](#repository-examples)

## Start Here

> [!IMPORTANT]
> For targets created with `boza_add_executable(...)`, you export an `App` subclass from your target module. The build system generates the entry point and calls `init()` / `run()` automatically.

If you are new to the public surface, read in this order:

1. [`boza.app`](modules/boza.app.md)
2. [`boza.ecs`](modules/boza.ecs.md)
3. [`boza.gfx`](modules/boza.gfx.md)
4. [`boza.input`](modules/boza.input.md)
5. [`boza.core`](modules/boza.core.md)
6. [`boza.common`](modules/boza.common.md)

## What `import boza;` gives you

<details>
<summary>Umbrella module contents</summary>

`import boza;` re-exports:

- `boza.app`
- `boza.common`
- `boza.core`
- `boza.ecs`
- `boza.input`
- `boza.gfx`

Modules such as `boza.platform`, `boza.rhi`, and `boza.detail` exist in the repository, but they are internal engine layers rather than the main gameplay-facing surface.

</details>

## Module Guides

| Guide | Best for | Read it when you need... |
| --- | --- | --- |
| [`boza.app`](modules/boza.app.md) | app definition and runtime control | startup flow, `App`, fullscreen, cursor, or FPS control |
| [`boza.common`](modules/boza.common.md) | shared helpers and convenience exports | `Flags<T>`, `Random`, property wrappers, container aliases, or GLM helpers |
| [`boza.core`](modules/boza.core.md) | always-on runtime services | logging, assertions, or frame timing |
| [`boza.ecs`](modules/boza.ecs.md) | gameplay structure | scenes, game objects, transforms, and system stages |
| [`boza.gfx`](modules/boza.gfx.md) | rendering and compute | meshes, materials, textures, buffers, cameras, renderers, or compute dispatch |
| [`boza.input`](modules/boza.input.md) | player and editor input | keys, polling, bindings, cursor modes, or callback capture |

## Choose by Task

| I want to... | Start here |
| --- | --- |
| export a Boza app target | [`boza.app`](modules/boza.app.md) |
| create a scene and spawn objects | [`boza.ecs`](modules/boza.ecs.md) |
| move objects with frame time | [`boza.core`](modules/boza.core.md) and [`boza.ecs`](modules/boza.ecs.md) |
| attach camera or mesh rendering | [`boza.gfx`](modules/boza.gfx.md) |
| bind keys or mouse callbacks | [`boza.input`](modules/boza.input.md) |
| use flags, random helpers, JSON, or GLM conveniences | [`boza.common`](modules/boza.common.md) |
| feed shader parameters or run compute work | [`boza.gfx`](modules/boza.gfx.md) |

## Repository Examples

<details>
<summary>Example targets in this repository</summary>

- `examples/material_showcase`
- `examples/instancing_example`
- `examples/shooting_example`
- `examples/serialization_showcase`

</details>

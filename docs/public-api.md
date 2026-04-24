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

1. [`boza.app`](modules/boza.app/README.md)
2. [`boza.ecs`](modules/boza.ecs/README.md)
3. [`boza.gfx`](modules/boza.gfx/README.md)
4. [`boza.input`](modules/boza.input/README.md)
5. [`boza.core`](modules/boza.core/README.md)
6. [`boza.common`](modules/boza.common/README.md)

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
| [`boza.app`](modules/boza.app/README.md) | app definition and runtime control | startup flow, `App`, fullscreen, cursor, or FPS control |
| [`boza.common`](modules/boza.common/README.md) | shared helpers and convenience exports | `Flags<T>`, property wrappers, container aliases, or GLM helpers |
| [`boza.core`](modules/boza.core/README.md) | always-on runtime services | logging, assertions, random utilities, or frame timing |
| [`boza.ecs`](modules/boza.ecs/README.md) | gameplay structure | scenes, game objects, transforms, and system stages |
| [`boza.gfx`](modules/boza.gfx/README.md) | rendering and compute | meshes, materials, textures, buffers, cameras, renderers, or compute dispatch |
| [`boza.input`](modules/boza.input/README.md) | player and editor input | keys, polling, bindings, cursor modes, or callback capture |

## Choose by Task

| I want to... | Start here |
| --- | --- |
| export a Boza app target | [`boza.app`](modules/boza.app/README.md) |
| create a scene and spawn objects | [`boza.ecs`](modules/boza.ecs/README.md) |
| move objects with frame time | [`boza.core`](modules/boza.core/README.md) and [`boza.ecs`](modules/boza.ecs/README.md) |
| attach camera or mesh rendering | [`boza.gfx`](modules/boza.gfx/README.md) |
| bind keys or mouse callbacks | [`boza.input`](modules/boza.input/README.md) |
| use flags, JSON, GLM conveniences, or property wrappers | [`boza.common`](modules/boza.common/README.md) |
| use random helpers | [`boza.core`](modules/boza.core/README.md) |
| feed shader parameters or run compute work | [`boza.gfx`](modules/boza.gfx/README.md) |

## Repository Examples

<details>
<summary>Example targets in this repository</summary>

- `examples/material_showcase`
- `examples/instancing_example`
- `examples/shooting_example`
- `examples/serialization_showcase`

</details>

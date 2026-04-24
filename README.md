# Boza

Boza is a Windows-first C++23 game engine prototype built around C++ modules, a scene/ECS workflow, and a Vulkan-first rendering stack.

> [!IMPORTANT]
> The supported gameplay-facing surface lives in `boza/public` and is re-exported by `import boza;`.

## Contents

- [Overview](#overview)
- [Documentation](#documentation)
- [Executable Pattern](#executable-pattern)
- [Public Module Map](#public-module-map)
- [Repository Layout](#repository-layout)
- [Examples](#examples)
- [Build and Run](#build-and-run)
- [Current Status](#current-status)

## Overview

Boza currently focuses on:

- C++23 modules as the public engine surface
- a scene-graph + ECS gameplay model built around `Scene`, `GameObject`, and `Transform`
- a Vulkan-first graphics stack with public rendering and compute APIs
- name-based resource workflows for meshes, materials, textures, samplers, and buffers
- runnable example targets under `examples/`

## Documentation

Documentation entry points:

| Page | Purpose |
| --- | --- |
| [`docs/README.md`](docs/README.md) | documentation index and reading paths |
| [`docs/public-api.md`](docs/public-api.md) | public API landing page |
| [`docs/modules/README.md`](docs/modules/README.md) | per-module reference index |

## Executable Pattern

> [!NOTE]
> Inside this repository, targets created with `boza_add_executable(...)` should export an `App` subclass. The generated entry point constructs that class and calls `init()` / `run()` automatically.

Typical target module:

```cpp
export module hello_boza;

import boza;

using namespace boza;

export class HelloBoza : public App
{
protected:
    void setup() override
    {
        Scene::main = Scene::create("MainScene");

        GameObject camera = GameObject::create("MainCamera", Scene::persistent());
        auto& camera_transform = camera.get_component<Transform>();
        camera_transform.local_position = glm::vec3{ 0.0f, 2.0f, -6.0f };
        camera_transform.look_at(glm::vec3{ 0.0f, 0.0f, 0.0f });

        auto& camera_component = camera.add_component<Camera>();
        camera_component.is_primary = true;
    }
};
```

Target authoring rules:

1. Define `export module <target>;`
2. Export an app class whose name matches the target converted from `snake_case` to `PascalCase`
3. Use `boza_add_executable(<target>)` in that target's `CMakeLists.txt`

## Public Module Map

| Module | Primary role | Detailed guide |
| --- | --- | --- |
| `boza.app` | app definition and runtime control | [`docs/modules/boza.app.md`](docs/modules/boza.app.md) |
| `boza.common` | shared helpers, aliases, flags, properties, and GLM utilities | [`docs/modules/boza.common.md`](docs/modules/boza.common.md) |
| `boza.core` | logging, assertions, random utilities, and time state | [`docs/modules/boza.core.md`](docs/modules/boza.core.md) |
| `boza.ecs` | scenes, game objects, transforms, and system stages | [`docs/modules/boza.ecs.md`](docs/modules/boza.ecs.md) |
| `boza.gfx` | rendering resources, render-facing components, and compute | [`docs/modules/boza.gfx.md`](docs/modules/boza.gfx.md) |
| `boza.input` | keys, polling, bindings, cursor control, and callback capture | [`docs/modules/boza.input.md`](docs/modules/boza.input.md) |

## Repository Layout

```text
.
|- boza/
|  |- public/              public engine modules
|  \- src/                engine implementation and backends
|- docs/                  documentation set
|- examples/              runnable example targets
|- shaders/               shader source files
\- tools/shader_processor shader processing tool
```

## Examples

Example targets wired into the root build:

| Target | Focus |
| --- | --- |
| `material_showcase` | materials, compute-generated textures, free-fly camera control |
| `instancing_example` | instanced rendering |
| `shooting_example` | gameplay systems, input, combat logic |
| `serialization_showcase` | serialization-oriented workflows |

## Build and Run

See [`BUILD.md`](BUILD.md) for prerequisites and full setup instructions.

Short version:

1. Install the Windows/MSVC prerequisites from [`BUILD.md`](BUILD.md)
2. Configure with the `default` CMake preset
3. Build with the `Debug` or `Release` build preset
4. Run the executable from its generated output directory so assets and shaders resolve correctly

## Current Status

- Windows + MSVC is the supported development path today
- Vulkan is the primary graphics path at the moment
- `game/` is still disabled in the root `CMakeLists.txt`
- Internal modules under `boza/src` are implementation detail rather than stable gameplay API

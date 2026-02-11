# Boza
Boza is a small C++23 game engine prototype.

## Features
- Modular engine layout (`app`, `common`, `core`, `ecs`, `gfx`, `input`, `platform`, `rhi`)
- Vulkan-first rendering with an abstract rendering hardware interface (RHI)
- Platform/window layer for creating windows and polling input events
- Shader processing tool (`tools/shader_processor`) integrated into CMake; outputs SPIR-V and platform variants
- Lightweight scene/Entity-Component framework and `GameLoop`
- Examples (`examples/`) demonstrating scene setup and systems

## My goals for this project:
- To make a simple lightweight engine that has the essential functionality (rendering, audio, input, scene management, ECS, materials, etc.) without unnecessary bloat
- To be straightforward to access low-level functionality (like writing custom shaders)
- To provide a clean modular architecture that can be extended in the future

## Build & Run
See [BUILD.md](BUILD.md) for detailed build and setup instructions.

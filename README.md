# Boza
Boza is a small C++23 game engine prototype.

## Features
- Modular engine layout (`common`, `core`, `platform`, `rhi`, `ahi`, `src`)
- Vulkan-first rendering with an abstract rendering hardware interface
- Platform/window layer for creating and polling a window
- Shader processing tool (`tools/shader_processor`) integrated into CMake; outputs SPIR-V and platform variants
- Lightweight scene/Entity-Component framework and `GameLoop`
- Sample game (`game/`) demonstrating scene setup and behavior scripts

## My goals for this project:
- To make a simple lightweight engine that has the essential functionality (rendering, audio, input, scene management, ECS, materials, etc.) to prevent unnecessary bloat
- To be straightforward to access low-level functionality (like writing custom shaders and defining custom render passes)
- To provide a clean modular architecture that can be extended in the future
- To be cross-platform
- To be testable (unit tests, integration tests, load tests, etc.)

## Build & Run
See [BUILD.md](BUILD.md) for detailed build and setup instructions.

## Documentation
- [Structure](docs/structure.md): Detailed project and engine module structure with diagrams.
- [Roadmap](docs/roadmap.md): Current work and future plans.

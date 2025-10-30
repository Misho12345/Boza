# Project Structure & Modules

This file explains how the project is constructed and how the engine modules relate to each other.

Top-level modules
- `common/` — pch, asset path resolver
- `core/` — the core engine facilities (GameLoop, Scene, GameObject, Components, Logger, etc.).
- `platform/` — window creation
- `rhi/` — Rendering Hardware Interface: abstraction layer for graphics APIs (only Vulkan is currently implemented).
- `ahi/` — Audio Hardware Interface: abstraction for audio backends (currently just the skeleton).
- `src/` — engine dll entry points and higher-level subsystems.
- `shaders/` and `tools/shader_processor` — shader sources and the processing tool used by the build to produce SPIR-V and other variants.
- `game/` — sample game using the engine and demonstrating scene setup, example behaviors, materials, etc.

How modules interact

```mermaid
graph TD
    %% External Dependencies
    glm[glm]
    spdlog[spdlog]
    entt[EnTT]
    json[nlohmann_json]
    glfw[GLFW]
    stb[STB]
    
    %% Base Interface
    glm --> boza_common
    spdlog --> boza_common
    entt --> boza_common
    json --> boza_common
    
    %% Core Interface Layer
    boza_common --> boza_core_interface
    boza_common --> boza_platform_interface
    
    %% Implementation Layer
    boza_core_interface --> boza_core
    
    boza_platform_interface --> boza_platform
    boza_core_interface --> boza_platform
    glfw --> boza_platform
    
    %% RHI Layer
    boza_common --> boza_rhi_interface
    boza_core_interface --> boza_rhi_interface
    boza_platform_interface --> boza_rhi_interface
    stb --> boza_rhi_interface
    
    boza_rhi_interface --> boza_rhi
    boza_rhi_interface --> boza_rhi_vulkan
    boza_rhi_interface --> boza_rhi_opengl
    
    boza_rhi_vulkan -.-> boza_rhi
    boza_rhi_opengl -.-> boza_rhi
    
    %% AHI Layer
    boza_common --> boza_ahi_interface
    boza_core_interface --> boza_ahi_interface
    boza_platform_interface --> boza_ahi_interface
    
    boza_ahi_interface --> boza_ahi
    boza_ahi_interface --> boza_ahi_openal
    
    boza_ahi_openal -.-> boza_ahi
    
    %% Final Engine
    boza_common --> boza_engine
    boza_core_interface --> boza_engine
    boza_core --> boza_engine
    boza_platform --> boza_engine
    boza_rhi --> boza_engine
    boza_ahi --> boza_engine
    
    %% Tools
    shader_processor --> compile_shaders
    
    %% Game Executable
    boza_engine --> boza_game
    compile_shaders --> boza_game
```

Rendering Flow

```mermaid
graph TB
    A[Initialize Rendering] --> B[Create Instance]
    B --> C[Setup Device]
    C --> D[Create Swapchain]
    D --> E[Load Materials]
    E --> F[Allocate Descriptors]
    
    G[Start Frame] --> H[Acquire Next Image]
    H --> I[Begin Recording]
    I --> J[Sort by Shader]
    
    J --> K[Build Pipeline]
    K --> L[Load Shaders]
    
    L --> M[Process Objects]
    M --> N[Upload Mesh Data]
    M --> O[Bind Resources]
    O --> P[Update Uniforms]
    P --> Q[Set Transforms]
    Q --> R[Issue Draw Call]
    
    R --> S{More Objects?}
    S -->|Yes| M
    S -->|No| T[Finish Recording]
    
    T --> U[Submit Commands]
    U --> V[Display Frame]
    V --> G
    
    C -.-> X[Queue Management]
    D -.-> Y[Image + Sync Resources]
```

Notes
- The `rhi` provides an API for creating buffers, textures, pipelines, descriptor sets, command buffers, and submitting work. The intention is to let the `src`/`core` subsystems remain backend-agnostic.
- A similar approach will be taken for the `ahi` subsystems.
- Shader processing is integrated into the CMake flow so shaders are precompiled into SPIR-V and platform variations during the build.

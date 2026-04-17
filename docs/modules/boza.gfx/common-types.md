# Common Types and Settings

[<- Previous](README.md) | [Up](README.md) | [Next ->](mesh.md)

Module: `boza.gfx`

Declared in: `boza/public/gfx/common.ixx`

## Contents

- [Overview](#overview)
- [Resource Access and Buffer Types](#resource-access-and-buffer-types)
- [Pipeline State Enums](#pipeline-state-enums)
- [Texture and Sampler Enums](#texture-and-sampler-enums)
- [Data Types and Metadata](#data-types-and-metadata)

## Overview

`boza.gfx.common` contains the shared vocabulary used by the rest of the graphics API.

## Resource Access and Buffer Types

| Type | Purpose |
| --- | --- |
| `ResourceAccessMode` | static vs dynamic backing behavior |
| `BufferUsage` | vertex, index, uniform, storage, staging |
| `Vertex` | common mesh vertex format (`position`, `normal`, `tex_coord`) |

## Pipeline State Enums

| Enum | Purpose |
| --- | --- |
| `CompareOp` | depth / compare operation |
| `CullMode` | rasterization culling mode |
| `FrontFace` | winding order interpretation |

## Texture and Sampler Enums

| Enum | Purpose |
| --- | --- |
| `TextureLayout` | texture layout / state transitions |
| `TextureFormat` | public texture formats |
| `TextureUsage` | texture usage flags |
| `TextureType` | 1D / 2D / 3D / array / cube texture kind |
| `SamplerFilter` | nearest / linear / anisotropic filtering |
| `SamplerWrap` | repeat / clamp / mirror addressing |

## Data Types and Metadata

| Type | Purpose |
| --- | --- |
| `ShaderDataType` | reflected shader-side data type classification |
| `BindingInfo` | reflected binding metadata used by materials and compute |
| `get_shader_data_type<T>()` | maps C++ types to `ShaderDataType` |

[<- Previous](README.md) | [Up](README.md) | [Next ->](mesh.md)

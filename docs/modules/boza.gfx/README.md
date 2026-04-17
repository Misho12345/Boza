# boza.gfx

[<- Previous](../boza.ecs/systems.md) | [Up](../README.md) | [Next ->](common-types.md)

Module: `boza.gfx`

Declared in: `boza/public/gfx/gfx.ixx`

## Overview

`boza.gfx` is the largest public rendering-facing module. It exposes shared graphics vocabulary, resource registries, runtime buffers, render-facing scene components, and compute dispatch helpers.

> [!TIP]
> Most render and compute bindings are string-based and driven by shader reflection. Treat names in code and names in shader metadata as the same contract.

## Contents

- [Main Areas](#main-areas)
- [Reference Pages](#reference-pages)

## Main Areas

| Area | Purpose |
| --- | --- |
| shared graphics vocabulary | enums, data types, vertex layout structs, binding metadata |
| named resources | `Mesh`, `Material`, `Texture`, `Sampler` |
| runtime GPU data | `Buffer` |
| render-facing scene components | `Camera`, `MeshRenderer` |
| compute | `ComputeDispatcher`, `ComputeDispatchGroup` |

## Reference Pages

| Page | Covers |
| --- | --- |
| [`Common Types and Settings`](common-types.md) | `ResourceAccessMode`, `ShaderDataType`, buffer / texture / sampler enums, `Vertex`, `BindingInfo`, helper mappings |
| [`Mesh`](mesh.md) | mesh registry, bounds, revisions, and lookup surface |
| [`Material`](material.md) | material registry, settings, binding model, `PropertyBinder`, push constants |
| [`Texture`](texture.md) | texture settings, creation, loading, upload, staging, read-back, layout transitions |
| [`Sampler`](sampler.md) | sampler creation, lookup, and state accessors |
| [`Buffer`](buffer.md) | upload, staging, read-back, mapping, dynamic vs static access |
| [`Camera`](camera.md) | camera component projection settings and primary-camera flag |
| [`MeshRenderer`](mesh-renderer.md) | mesh/material assignment by name or object reference |
| [`Compute`](compute.md) | compute dispatcher and grouped compute submission |

[<- Previous](../boza.ecs/systems.md) | [Up](../README.md) | [Next ->](common-types.md)

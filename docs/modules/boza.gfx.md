# boza.gfx

`boza.gfx` is the public graphics and rendering module. It exposes shared rendering vocabulary, resource APIs, render-facing scene components, and compute helpers.

> [!TIP]
> Most render and compute bindings are string-based and driven by shader reflection. Treat names in code and names in shader metadata as the same contract.

## Contents

- [Overview](#overview)
- [Module Synopsis](#module-synopsis)
- [Reference](#reference)
  - [Shared Types](#shared-types)
  - [Resources](#resources)
  - [Scene Components](#scene-components)
  - [Compute](#compute)
- [Contracts](#contracts)

## Overview

This module provides the public rendering-facing surface for Boza:

- shared graphics enums and POD types
- named registries for meshes, materials, textures, and samplers
- runtime GPU buffers
- camera and mesh-renderer scene components
- compute dispatch helpers

## Module Synopsis

| Item | Value |
| --- | --- |
| Primary import | `import boza.gfx;` |
| Shared vocabulary | `boza.gfx.common` |
| Registries | `Mesh`, `Material`, `Texture`, `Sampler` |
| Runtime resource | `Buffer` |
| Scene components | `Camera`, `MeshRenderer` |
| Compute | `ComputeDispatcher`, `ComputeDispatchGroup` |

## Reference

### Shared Types

`boza.gfx.common` contains the shared vocabulary used by the rest of the graphics API.

#### Common Enums and POD Types

- `Vertex`
- `BindingInfo`
- `ResourceAccessMode`
- `ShaderDataType`
- `BufferUsage`
- `CompareOp`
- `CullMode`
- `FrontFace`
- `TextureLayout`
- `TextureFormat`
- `TextureUsage`
- `TextureType`
- `SamplerFilter`
- `SamplerWrap`
- `get_shader_data_type<T>()`

### Resources

#### `Mesh`

```cpp
class Mesh final;
```

| Member | Purpose |
| --- | --- |
| `Mesh::create(name, vertices, indices)` | create a named mesh |
| `Mesh::replace(name, vertices, indices)` | replace or create a named mesh |
| `Mesh::get(name)` / `try_get(name)` | lookup by name |
| `Mesh::exists(name)` / `exists(ptr)` | existence checks |
| `name`, `vertices`, `indices`, `bounds` | read-only property-style access |
| `revision()` | mesh revision counter |

#### `Material`

```cpp
class Material;
struct MaterialSettings;
class PropertyBinder;
```

##### Registry Surface

| Member | Purpose |
| --- | --- |
| `Material::create(name, settings)` | create a named material |
| `Material::get(name)` / `try_get(name)` | lookup by name |
| `Material::destroy(name)` | destroy by name |

##### Binding Surface

| Member | Purpose |
| --- | --- |
| `material["binding_name"] = value` | reflected property assignment |
| `update_property(name, value)` | update reflected value data |
| `update_texture(name, texture)` | bind a texture |
| `update_texture(name, texture, sampler)` | bind a texture and sampler |
| `update_buffer(name, buffer)` | bind a buffer |
| `push_constants(name, value)` | write push constant data |
| `lookup_binding(name)` | inspect reflected binding metadata |

#### `Texture`

```cpp
class Texture final;
struct TextureSettings;
```

| Member | Purpose |
| --- | --- |
| `Texture::create(name, settings)` | create a named texture |
| `Texture::copy(src_name, dst_name, access_mode)` | copy a named texture |
| `Texture::get_or_load(name)` | load or reuse a texture |
| `Texture::get_or_load_cubemap(name)` | load or reuse a cubemap |
| `Texture::get(name)` / `try_get(name)` / `destroy(name)` | registry access |
| `upload(...)`, `upload_layer(...)`, `upload_from(...)` | populate texture data |
| `stage(...)`, `read_back()`, `save_to_file(path)` | read-back and staging helpers |
| `transition_layout(old_layout, new_layout)` | explicit layout transition |

#### `Sampler`

```cpp
class Sampler final;
```

| Member | Purpose |
| --- | --- |
| `Sampler::create(name, ...)` | create a named sampler |
| `Sampler::get(name)` / `try_get(name)` | lookup by name |
| `Sampler::destroy(name)` | destroy by name |
| `filter`, `wrap_u`, `wrap_v`, `wrap_w`, `mipmap_mode`, `mip_lod_bias`, `min_lod`, `max_lod`, `max_anisotropy` | property-style accessors |

#### `Buffer`

```cpp
class Buffer final;
```

| Member | Purpose |
| --- | --- |
| `Buffer(size, usage, access_mode)` | create a GPU buffer |
| `upload(...)` | upload raw or typed data |
| `upload_from(staging_buffer, ...)` | copy from another buffer |
| `read_back(...)` / `read_back()` | read data back to CPU memory |
| `stage(...)` | create a staging copy |
| `map()` / `unmap()` | mapped access |
| `size`, `access_mode` | property-style accessors |

### Scene Components

#### `Camera`

```cpp
class Camera final;
```

| Member | Purpose |
| --- | --- |
| `projection_type` | perspective or orthographic mode |
| `fov` | perspective field of view |
| `near_clip`, `far_clip` | clip distances |
| `ortho_size` | orthographic size |
| `is_primary` | primary camera property |
| `projection_matrix(aspect_ratio)` | build the projection matrix |

#### `MeshRenderer`

```cpp
class MeshRenderer final;
```

| Member | Purpose |
| --- | --- |
| `mesh` / `material` | resolved object references |
| `mesh_name` / `material_name` | name-based bindings |

### Compute

#### `ComputeDispatcher`

```cpp
class ComputeDispatcher;
```

| Member | Purpose |
| --- | --- |
| constructor with shader name | create a dispatcher for one compute shader |
| `set(name, value)` | bind POD data |
| `set(name, texture)` | bind a texture |
| `set(name, buffer)` | bind a buffer |
| `dispatch(...)` | dispatch by output dimensions |
| `dispatch_groups(...)` | dispatch by explicit workgroup counts |
| `wait()` | wait for completion |
| `status()`, `working()`, `finished()`, `failed()` | query execution state |
| `work_group_size()` | inspect reflected workgroup size |

#### `ComputeDispatchGroup`

```cpp
class ComputeDispatchGroup;
```

| Member | Purpose |
| --- | --- |
| `add(...)` / `add_groups(...)` | append compute steps |
| `clear()` | clear recorded steps |
| `record()` | record the dispatch list |
| `submit()` | submit recorded work |
| `wait()` | wait for completion |
| `run()` | record, submit, and wait |
| `status()`, `working()`, `finished()`, `failed()` | query execution state |

## Contracts

- Most resource APIs are name-based and registry-backed.
- `ResourceAccessMode::Dynamic` usually means per-frame backing resources under the hood.
- `RenderingSystem` is exported, but most gameplay code should treat it as engine-managed plumbing rather than call it directly.
- Graphics and resource creation assumes the render context is already initialized, so create resources from or after `App::setup()`.

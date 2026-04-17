# MeshRenderer

[<- Previous](camera.md) | [Up](README.md) | [Next ->](compute.md)

Module: `boza.gfx`

Declared in: `boza/public/gfx/components/mesh_renderer.ixx`

## Contents

- [Synopsis](#synopsis)
- [Property Surface](#property-surface)
- [Usage Notes](#usage-notes)

## Synopsis

```cpp
class MeshRenderer final;
```

## Property Surface

| Member | Purpose |
| --- | --- |
| `mesh` | resolved mesh reference |
| `material` | resolved material reference |
| `mesh_name` | assign mesh by registry name |
| `material_name` | assign material by registry name |

## Usage Notes

- Assigning by name is the usual public-facing workflow.
- Assigning by object reference is also supported where an actual `Mesh&` or `Material&` is already available.
- The renderer cooperates with engine-side render caching and late resolution of named assets.

[<- Previous](camera.md) | [Up](README.md) | [Next ->](compute.md)

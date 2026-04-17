# Mesh

[<- Previous](common-types.md) | [Up](README.md) | [Next ->](material.md)

Module: `boza.gfx`

Declared in: `boza/public/gfx/mesh.ixx`

## Contents

- [Synopsis](#synopsis)
- [Registry Surface](#registry-surface)
- [Read Surface](#read-surface)
- [Notes](#notes)

## Synopsis

```cpp
class Mesh final;
struct BoundingSphere final;
```

## Registry Surface

| Member | Purpose |
| --- | --- |
| `Mesh::create(name, vertices, indices)` | create a named mesh |
| `Mesh::replace(name, vertices, indices)` | replace or create a named mesh |
| `Mesh::get(name)` / `try_get(name)` | lookup by name |
| `Mesh::exists(name)` / `exists(ptr)` | existence checks |

## Read Surface

| Member | Purpose |
| --- | --- |
| `name` | mesh name |
| `vertices` | read-only vertex data |
| `indices` | read-only index data |
| `bounds` | computed bounding sphere |
| `revision()` | revision counter used by rendering cache code |

## Notes

- Meshes are registry-backed and keyed by name.
- `replace(...)` is the intended path when you want to mutate a named mesh and bump its revision.

[<- Previous](common-types.md) | [Up](README.md) | [Next ->](material.md)

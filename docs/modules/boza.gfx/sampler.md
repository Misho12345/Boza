# Sampler

[<- Previous](texture.md) | [Up](README.md) | [Next ->](buffer.md)

Module: `boza.gfx`

Declared in: `boza/public/gfx/sampler.ixx`

## Contents

- [Synopsis](#synopsis)
- [Registry Surface](#registry-surface)
- [Read Surface](#read-surface)

## Synopsis

```cpp
class Sampler final;
```

## Registry Surface

| Member | Purpose |
| --- | --- |
| `Sampler::create(name, ...)` | create a named sampler |
| `Sampler::get(name)` / `try_get(name)` | lookup by name |
| `Sampler::destroy(name)` | destroy by name |

## Read Surface

| Member | Purpose |
| --- | --- |
| `filter` | sampler filter |
| `wrap_u`, `wrap_v`, `wrap_w` | address modes |
| `mipmap_mode` | mip filtering mode |
| `mip_lod_bias`, `min_lod`, `max_lod`, `max_anisotropy` | LOD and anisotropy controls |
| `name()` | sampler name |

[<- Previous](texture.md) | [Up](README.md) | [Next ->](buffer.md)

# Camera

[<- Previous](buffer.md) | [Up](README.md) | [Next ->](mesh-renderer.md)

Module: `boza.gfx`

Declared in: `boza/public/gfx/components/camera.ixx`

## Contents

- [Synopsis](#synopsis)
- [Fields and Properties](#fields-and-properties)
- [Methods](#methods)

## Synopsis

```cpp
class Camera final;
```

## Fields and Properties

| Member | Purpose |
| --- | --- |
| `projection_type` | perspective or orthographic mode |
| `fov` | perspective field of view |
| `near_clip`, `far_clip` | clip distances |
| `ortho_size` | orthographic size |
| `is_primary` | primary-camera property |

## Methods

| Member | Purpose |
| --- | --- |
| `projection_matrix(aspect_ratio)` | build the projection matrix |

[<- Previous](buffer.md) | [Up](README.md) | [Next ->](mesh-renderer.md)

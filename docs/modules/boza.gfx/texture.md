# Texture

[<- Previous](material.md) | [Up](README.md) | [Next ->](sampler.md)

Module: `boza.gfx`

Declared in: `boza/public/gfx/texture.ixx`

## Contents

- [Synopsis](#synopsis)
- [TextureSettings](#texturesettings)
- [Registry Surface](#registry-surface)
- [Instance Surface](#instance-surface)

## Synopsis

```cpp
struct TextureSettings final;
class Texture final;
```

## TextureSettings

| Field | Purpose |
| --- | --- |
| `type` | texture kind |
| `format` | texture format |
| `access_mode` | static or dynamic backing |
| `width`, `height`, `depth` | dimensions |
| `usage_flags` | usage flags |

## Registry Surface

| Member | Purpose |
| --- | --- |
| `Texture::copy(src_name, dst_name, access_mode)` | copy a named texture |
| `Texture::create(name, settings)` | create a named texture |
| `Texture::get_or_load(name)` | load or reuse a texture |
| `Texture::get_or_load_cubemap(name)` | load or reuse a cubemap |
| `Texture::get(name)` / `try_get(name)` | lookup by name |
| `Texture::exists(ptr)` | validity test for a texture pointer |
| `Texture::destroy(name)` | destroy by name |

## Instance Surface

| Member | Purpose |
| --- | --- |
| `upload(...)`, `upload_layer(...)`, `upload_from(...)` | populate texture data |
| `stage(...)` | create a staging buffer copy |
| `read_back()` | read all texture data back |
| `save_to_file(path)` | write back to disk |
| `transition_layout(old_layout, new_layout)` | explicit layout transition |
| `width`, `height`, `depth`, `type`, `format` | property-style accessors |
| `is_valid()` | validity test |
| `name()` | texture name |

[<- Previous](material.md) | [Up](README.md) | [Next ->](sampler.md)

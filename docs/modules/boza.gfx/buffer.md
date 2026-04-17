# Buffer

[<- Previous](sampler.md) | [Up](README.md) | [Next ->](camera.md)

Module: `boza.gfx`

Declared in: `boza/public/gfx/buffer.ixx`

## Contents

- [Synopsis](#synopsis)
- [Construction](#construction)
- [Upload and Read-Back](#upload-and-read-back)
- [Mapping and Introspection](#mapping-and-introspection)

## Synopsis

```cpp
class Buffer final;
```

## Construction

| Member | Purpose |
| --- | --- |
| `Buffer(size, usage, access_mode)` | create a GPU buffer |

## Upload and Read-Back

| Member | Purpose |
| --- | --- |
| `upload(const void* data, size, offset)` | upload raw data |
| `upload(const T& data)` | upload a single typed object |
| `upload(std::span<T> data)` | upload a contiguous typed span |
| `upload_from(staging_buffer, ...)` | copy from another buffer |
| `read_back(void* data, size, offset)` | read into user memory |
| `read_back()` | return a byte vector |
| `stage(...)` | create a staging copy |

## Mapping and Introspection

| Member | Purpose |
| --- | --- |
| `map()` / `unmap()` | mapped access |
| `size` | property-style size accessor |
| `access_mode` | property-style access-mode accessor |

[<- Previous](sampler.md) | [Up](README.md) | [Next ->](camera.md)

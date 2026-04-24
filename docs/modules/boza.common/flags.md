# Flags<T>

[<- Previous](README.md) | [Up](README.md) | [Next ->](properties.md)

Module: `boza.common`

Declared in: `boza/public/common/flags.ixx`

## Contents

- [Synopsis](#synopsis)
- [Operations](#operations)
- [Usage Notes](#usage-notes)
- [Example](#example)

## Synopsis

```cpp
template <typename T>
class Flags final;
```

`Flags<T>` is Boza's strongly typed enum-bitflag wrapper.

## Operations

| Member | Purpose |
| --- | --- |
| `operator|`, `operator&`, `operator^`, `operator~` | combine and transform flag sets |
| `operator|=`, `operator&=`, `operator^=` | mutate an existing flag set |
| `operator==`, `operator!=` | compare two flag sets |
| `any()` | returns true if any flags are set |
| `none()` | returns true if no flags are set |
| `value()` | returns the underlying integral representation |
| `operator bool()` | shorthand for `any()` |

## Usage Notes

- `T` must be an enum type.
- `Flags<T>` is used by several public graphics enums such as `TextureUsage`.
- Some public enums provide helper overloads that directly produce `Flags<T>` values, for example `TextureUsage::Sampled | TextureUsage::TransferDst`.

## Example

```cpp
Flags<TextureUsage> usage = TextureUsage::Sampled | TextureUsage::TransferDst;

if (usage & TextureUsage::Sampled)
{
    Log::info("Texture can be sampled");
}
```

[<- Previous](README.md) | [Up](README.md) | [Next ->](properties.md)

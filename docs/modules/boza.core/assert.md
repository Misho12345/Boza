# assert

[<- Previous](log.md) | [Up](README.md) | [Next ->](random.md)

Module: `boza.core`

Declared in: `boza/public/core/assert.ixx`

## Contents

- [Synopsis](#synopsis)
- [Behavior](#behavior)
- [Example](#example)

## Synopsis

```cpp
template <typename... Args>
void assert(bool condition, std::format_string<Args...> fmt, Args... args);
```

## Behavior

- in `_DEBUG`, failed assertions log and abort
- outside `_DEBUG`, the helper becomes a no-op

> [!WARNING]
> Treat `boza::assert(...)` as development-time validation, not release-time enforcement.

## Example

```cpp
boza::assert(player.valid(), "Player must exist before entering combat");
```

[<- Previous](log.md) | [Up](README.md) | [Next ->](random.md)

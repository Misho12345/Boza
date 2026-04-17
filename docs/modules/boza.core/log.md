# Log

[<- Previous](README.md) | [Up](README.md) | [Next ->](assert.md)

Module: `boza.core`

Declared in: `boza/public/core/log.ixx`

## Contents

- [Synopsis](#synopsis)
- [Levels](#levels)
- [Usage Patterns](#usage-patterns)

## Synopsis

```cpp
class Log final;
```

## Levels

| Level | Typical use |
| --- | --- |
| `Log::trace(...)` | very verbose engine or gameplay flow |
| `Log::debug(...)` | debugging values and state |
| `Log::info(...)` | normal runtime information |
| `Log::warn(...)` | recoverable issues or suspicious state |
| `Log::error(...)` | failures that broke the intended operation |
| `Log::critical(...)` | fatal errors or immediate shutdown paths |

## Usage Patterns

Both value-style and format-style usage are supported.

```cpp
Log::info("Spawned {} objects", count);
Log::warn("Player health is low: {}", health);
Log::debug("Transform: {}", transform.world_matrix());
```

[<- Previous](README.md) | [Up](README.md) | [Next ->](assert.md)

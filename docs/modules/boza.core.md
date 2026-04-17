# boza.core

`boza.core` contains the always-on runtime services: logging, assertions, and frame timing.

> [!NOTE]
> `boza::assert(...)` is debug-only. Treat it as development-time validation, not release-time enforcement.

## Contents

- [Overview](#overview)
- [Module Synopsis](#module-synopsis)
- [Reference](#reference)
  - [`Log`](#log)
  - [`assert`](#assert)
  - [`Time`](#time)
- [Contracts](#contracts)

## Overview

This module provides the baseline runtime services that most gameplay code depends on:

- structured logging through `Log`
- runtime validation through `boza::assert(...)`
- frame time and clock state through `Time`

## Module Synopsis

| Item | Value |
| --- | --- |
| Primary import | `import boza.core;` |
| Logging | `Log` |
| Assertions | `boza::assert(...)` |
| Time state | `Time` |

## Reference

### `Log`

```cpp
class Log final;
```

| Level | Typical use |
| --- | --- |
| `Log::trace(...)` | very verbose engine or gameplay flow |
| `Log::debug(...)` | debugging values and state |
| `Log::info(...)` | normal runtime information |
| `Log::warn(...)` | recoverable issues or suspicious state |
| `Log::error(...)` | failures that broke the intended operation |
| `Log::critical(...)` | fatal errors or immediate shutdown paths |

Each level supports both value logging and `std::format`-style usage.

```cpp
Log::info("Spawned {} objects", count);
Log::warn("Player health is low: {}", health);
Log::debug("Transform: {}", transform.world_matrix());
```

### `assert`

```cpp
template <typename... Args>
void assert(bool condition, std::format_string<Args...> fmt, Args... args);
```

Use `boza::assert(...)` when a condition must hold during development.

```cpp
boza::assert(player.valid(), "Player must exist before entering combat");
```

Behavior:

- in `_DEBUG`, failed assertions log and abort
- outside `_DEBUG`, the helper becomes a no-op

### `Time`

```cpp
class Time final;
```

#### Queries

| Member | Purpose |
| --- | --- |
| `Time::time()` | scaled elapsed time |
| `Time::delta_time()` | scaled frame delta |
| `Time::unscaled_time()` | unscaled elapsed time |
| `Time::unscaled_delta_time()` | unscaled frame delta |
| `Time::frame_count()` | current frame counter |

#### Writable Globals

| Member | Purpose |
| --- | --- |
| `Time::fixed_delta_time` | current fixed-step interval |
| `Time::time_scale` | global time scaling factor |

Example:

```cpp
transform.local_position += velocity * Time::delta_time();
phase += Time::delta_time();
alpha = std::sin(Time::time());
```

## Contracts

- `Time::delta_time()` already includes `Time::time_scale`.
- `Time::unscaled_delta_time()` ignores `Time::time_scale` and is useful for UI or timing that should not slow down.
- `Time::fixed_delta_time` is managed by the engine loop from physics settings.

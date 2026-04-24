# Time

[<- Previous](random.md) | [Up](README.md) | [Next ->](../boza.ecs/README.md)

Module: `boza.core`

Declared in: `boza/public/core/time.ixx`

## Contents

- [Synopsis](#synopsis)
- [Queries](#queries)
- [Writable Globals](#writable-globals)
- [Examples](#examples)
- [Contracts](#contracts)

## Synopsis

```cpp
class Time final;
```

## Queries

| Member | Purpose |
| --- | --- |
| `Time::time()` | scaled elapsed time |
| `Time::delta_time()` | scaled frame delta |
| `Time::unscaled_time()` | unscaled elapsed time |
| `Time::unscaled_delta_time()` | unscaled frame delta |
| `Time::frame_count()` | current frame counter |

## Writable Globals

| Member | Purpose |
| --- | --- |
| `Time::fixed_delta_time` | current fixed-step interval |
| `Time::time_scale` | global time scaling factor |

## Examples

```cpp
transform.local_position += velocity * Time::delta_time();
phase += Time::delta_time();
alpha = std::sin(Time::time());
```

## Contracts

- `Time::delta_time()` already includes `Time::time_scale`.
- `Time::unscaled_delta_time()` ignores `Time::time_scale` and is useful for UI or timing that should not slow down.
- `Time::fixed_delta_time` is managed by the engine loop from physics settings.

[<- Previous](random.md) | [Up](README.md) | [Next ->](../boza.ecs/README.md)

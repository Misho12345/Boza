# Random

[<- Previous](assert.md) | [Up](README.md) | [Next ->](time.md)

Module: `boza.core`

Declared in: `boza/public/core/random.ixx`

## Contents

- [Synopsis](#synopsis)
- [Reference](#reference)
- [Examples](#examples)

## Synopsis

```cpp
class Random final;
```

`Random` is a static utility for gameplay-friendly randomness.

## Reference

| Member | Purpose |
| --- | --- |
| `reseed(seed)` / `reseed()` | reseed the random engine |
| `number<T>()` | random number from the default range for `T` |
| `range<T>(min, max)` | random number between two bounds |
| `chance(probability)` | random boolean with probability `p` |
| `shuffle(span)` | in-place shuffle |
| `pick(range)` | random element from a contiguous range |
| `string(length)` | random string from a supplied charset |
| `alphanumeric(length)` / `alpha(length)` / `numeric(length)` / `hex(length)` | common string generators |
| `pick_enum(...)` / `pick_enum_weighted(...)` | enum selection helpers |

## Examples

```cpp
glm::vec3 offset{
    Random::range<float>(-5.0f, 5.0f),
    Random::range<float>(0.0f, 3.0f),
    Random::range<float>(-5.0f, 5.0f)
};
```

[<- Previous](assert.md) | [Up](README.md) | [Next ->](time.md)

# boza.common

`boza.common` provides the shared utility layer used across the rest of the public API.

## Contents

- [Overview](#overview)
- [Module Synopsis](#module-synopsis)
- [Reference](#reference)
  - [Namespace Conveniences](#namespace-conveniences)
  - [`Flags<T>`](#flagst)
  - [`Random`](#random)
  - [`Property` and `GlobalProperty`](#property-and-globalproperty)
  - [Property Usage Patterns](#property-usage-patterns)
  - [Container Aliases](#container-aliases)
  - [GLM and JSON Re-exports](#glm-and-json-re-exports)
- [Contracts](#contracts)

## Overview

This module contains the common building blocks reused throughout Boza:

- filesystem and JSON conveniences
- hash-container aliases
- enum flag helpers
- random utilities
- the property model used by the public API
- GLM re-exports and formatters

## Module Synopsis

| Item | Value |
| --- | --- |
| Primary import | `import boza.common;` |
| Re-exported convenience names | `fs`, `json`, curated literals |
| Utility types | `Flags<T>`, `Random`, `Property`, `GlobalProperty` |
| Container aliases | `flat_map`, `flat_set`, `node_map`, `node_set`, `mt::...` |

## Reference

### Namespace Conveniences

| Export | Purpose |
| --- | --- |
| `fs` | alias for `std::filesystem` |
| `json` | alias for `nlohmann::json` |
| string literals | `std::string_literals` re-export |
| string-view literals | `std::string_view_literals` re-export |
| chrono literals | `std::chrono_literals` re-export |
| complex literals | `std::complex_literals` re-export |

### `Flags<T>`

```cpp
template <typename T>
class Flags final;
```

`Flags<T>` is Boza's strongly typed enum-bitflag wrapper.

#### Common Operations

| Operation | Purpose |
| --- | --- |
| `operator|`, `operator&`, `operator^`, `operator~` | combine and transform flags |
| `operator|=`, `operator&=`, `operator^=` | mutate a flag set |
| `any()` | returns true when at least one flag is set |
| `none()` | returns true when no flags are set |
| `value()` | returns the underlying integral representation |
| `operator bool()` | shorthand for `any()` |

Example:

```cpp
Flags<TextureUsage> usage = TextureUsage::Sampled | TextureUsage::TransferDst;

if (usage & TextureUsage::Sampled)
{
    Log::info("Texture can be sampled");
}
```

### `Random`

```cpp
class Random final;
```

`Random` is a static utility for common gameplay-friendly randomness.

#### Common Surface

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

Example:

```cpp
glm::vec3 offset{
    Random::range<float>(-5.0f, 5.0f),
    Random::range<float>(0.0f, 3.0f),
    Random::range<float>(-5.0f, 5.0f)
};
```

### `Property` and `GlobalProperty`

```cpp
template <typename Owner, auto... Methods>
class Property final;

template <auto... Functions>
class GlobalProperty final;
```

Boza uses method-backed properties heavily. This is why many public APIs read like ordinary field access.

Example:

```cpp
transform.local_position = glm::vec3{ 1.0f, 2.0f, 3.0f };
Scene::main = Scene::create("Gameplay");
App::target_fps = 120.0f;
```

Most users consume properties rather than define them.

#### What `Property` is for

Use `Property<Owner, ...>` for instance members that should feel like fields while still routing through getter and setter methods.

Typical examples in the public API:

- `Transform::local_position`
- `Transform::rotation`
- `GameObject::name`
- `MeshRenderer::material_name`

This means user code can stay concise:

```cpp
auto& transform = obj.get_component<Transform>();

transform.local_position = glm::vec3{ 0.0f, 2.0f, 0.0f };
transform.local_scale *= 2.0f;

GameObject player = GameObject::create("Player");
player.name = "Hero";
```

#### What `GlobalProperty` is for

Use `GlobalProperty<...>` for static or global engine state that should be accessed like a variable while still routing through static getter and setter functions.

Typical examples in the public API:

- `Scene::main`
- `Scene::persistent`
- `App::cursor_state`
- `App::target_fps`

Example:

```cpp
Scene::main = Scene::create("Gameplay");
App::cursor_state = CursorState::HiddenLocked;
App::target_fps = 120.0f;
```

#### What operations are supported

Both wrappers are designed to behave like the value exposed by their getter/setter pair.

Common patterns you can rely on:

- assignment: `transform.local_position = value;`
- reading: `glm::vec3 p = transform.local_position;`
- function-style access: `auto p = transform.local_position();`
- compound operations when supported by the underlying type: `counter += 1;`
- comparisons and arithmetic against the exposed value when valid for that type

Example:

```cpp
if (App::target_fps > 60.0f)
{
    Log::info("High framerate target enabled");
}

transform.local_position = transform.local_position + glm::vec3{ 1.0f, 0.0f, 0.0f };
```

## Property Usage Patterns

### Consuming existing properties

This is the normal user-facing case.

#### Instance property example

```cpp
GameObject camera = GameObject::create("MainCamera", Scene::persistent());

auto& transform = camera.get_component<Transform>();
transform.local_position = glm::vec3{ 0.0f, 2.0f, -6.0f };
transform.look_at(glm::vec3{ 0.0f, 0.0f, 0.0f });
```

#### Global property example

```cpp
Scene::main = Scene::create("Gameplay");
App::cursor_state = CursorState::Normal;
```

### Defining your own `Property`

If you want one of your own types to expose a field-like API, the pattern is:

1. write private getter/setter helpers
2. expose a `Property<Owner, &Owner::getter, &Owner::setter>` member
3. initialize the property with `this`

Example:

```cpp
class Health
{
    int get_current() const { return current_; }
    void set_current(const int value) { current_ = value; }

public:
    [[msvc::no_unique_address]]
    Property<
        Health,
        &Health::get_current,
        &Health::set_current
    > current{ this };

private:
    int current_{ 100 };
};
```

### Defining your own `GlobalProperty`

The pattern for static/global state is similar, but it uses static functions instead of instance methods.

Example:

```cpp
class Settings
{
    static int get_volume() { return volume_; }
    static void set_volume(const int value) { volume_ = value; }

public:
    static inline GlobalProperty<
        &Settings::get_volume,
        &Settings::set_volume
    > volume;

private:
    static inline int volume_{ 50 };
};
```

### Container Aliases

| Alias group | Members |
| --- | --- |
| single-threaded | `flat_map`, `flat_set`, `node_map`, `node_set` |
| parallel | `mt::flat_map`, `mt::flat_set`, `mt::node_map`, `mt::node_set` |

### GLM and JSON Re-exports

`boza.common` re-exports JSON and GLM conveniences so gameplay code can stay module-based.

Example:

```cpp
glm::vec3 position{ 1.0f, 2.0f, 3.0f };
Log::info("Position: {}", position);
```

## Contracts

- `boza.common` is intentionally convenience-heavy.
- `Property` and `GlobalProperty` are part of the public API style, not just an internal trick.
- If you define your own `Property<...>` members, read the constraints in `boza/public/common/property.ixx` first. The wrapper relies on owner-offset recovery and has layout caveats.

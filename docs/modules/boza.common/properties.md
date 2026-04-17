# Property and GlobalProperty

[<- Previous](flags.md) | [Up](README.md) | [Next ->](aliases-and-glm.md)

Module: `boza.common`

Declared in:

- `boza/public/common/property.ixx`
- `boza/public/common/global_property.ixx`

## Contents

- [Synopsis](#synopsis)
- [Conceptual Model](#conceptual-model)
- [Selection Rules](#selection-rules)
- [Using Existing Properties](#using-existing-properties)
- [Authoring Property](#authoring-property)
- [Authoring GlobalProperty](#authoring-globalproperty)
- [Contracts](#contracts)

## Synopsis

```cpp
template <typename Owner, auto... Methods>
class Property final;

template <auto... Functions>
class GlobalProperty final;
```

## Conceptual Model

Boza uses method-backed properties so public APIs can look field-like while still routing through methods.

Examples from the public surface:

```cpp
transform.local_position = glm::vec3{ 1.0f, 2.0f, 3.0f };
Scene::main = Scene::create("Gameplay");
App::target_fps = 120.0f;
```

`Property` is for instance members.

Typical public examples:

- `Transform::local_position`
- `Transform::rotation`
- `GameObject::name`
- `MeshRenderer::material_name`

`GlobalProperty` is for static or global state.

Typical public examples:

- `Scene::main`
- `Scene::persistent`
- `App::cursor_state`
- `App::target_fps`

## Selection Rules

The method packs are intentionally flexible.

- you do not need both a getter and a setter
- you only need at least one participating function
- you may provide multiple getters and/or multiple setters
- the wrapper chooses the most suitable getter or setter for the operation being performed

In practice, this allows different behavior depending on context:

- read operations can use a getter
- assignment can use the best-matching setter overload
- mutable operations can prefer a reference-returning getter when available
- rvalue contexts can prefer an rvalue-returning getter when available

## Using Existing Properties

### Instance Properties

```cpp
auto& transform = obj.get_component<Transform>();

transform.local_position = glm::vec3{ 0.0f, 2.0f, 0.0f };
transform.local_scale *= 2.0f;

GameObject player = GameObject::create("Player");
player.name = "Hero";
```

### Global Properties

```cpp
Scene::main = Scene::create("Gameplay");
App::cursor_state = CursorState::HiddenLocked;
App::target_fps = 120.0f;
```

### Supported Usage Patterns

| Pattern | Example |
| --- | --- |
| assignment | `transform.local_position = value;` |
| read / conversion | `glm::vec3 p = transform.local_position;` |
| call-style access | `auto p = transform.local_position();` |
| compound operation | `counter += 1;` |
| comparison / arithmetic | `if (App::target_fps > 60.0f)` |

## Authoring Property

Pattern:

1. write private getter / setter helpers
2. expose `Property<Owner, ...>`
3. initialize the property with `this`

Minimal example:

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

Multi-method example:

```cpp
class Inventory
{
    int get_count() const { return count_; }
    int& get_count_ref() { return count_; }

    void set_count(const int value) { count_ = value; }
    void set_count(const std::size_t value) { count_ = static_cast<int>(value); }

public:
    [[msvc::no_unique_address]]
    Property<
        Inventory,
        &Inventory::get_count,
        &Inventory::get_count_ref,
        &Inventory::set_count,
        &Inventory::set_count
    > count{ this };

private:
    int count_{ 0 };
};
```

## Authoring GlobalProperty

Pattern:

1. write static getter / setter helpers
2. expose `GlobalProperty<...>` as a static member

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

As with `Property`, multiple getters and setters can be provided.

## Contracts

- `Property` and `GlobalProperty` are part of the public API style, not just an internal trick.
- If you define your own `Property<...>` members, read the constraints in `boza/public/common/property.ixx` first. The wrapper relies on owner-offset recovery and has layout caveats.

[<- Previous](flags.md) | [Up](README.md) | [Next ->](aliases-and-glm.md)

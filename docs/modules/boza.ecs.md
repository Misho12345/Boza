# boza.ecs

`boza.ecs` is the main gameplay module. It exposes scenes, game objects, transforms, and the public staged-system DSL.

> [!IMPORTANT]
> Set `Scene::main` before relying on the shorthand `GameObject::create(name)` or `GameObject::find(name)`.

## Contents

- [Overview](#overview)
- [Module Synopsis](#module-synopsis)
- [Reference](#reference)
  - [`Scene`](#scene)
  - [`GameObject`](#gameobject)
  - [`Transform`](#transform)
  - [Systems DSL](#systems-dsl)
- [Contracts](#contracts)

## Overview

This module provides the core gameplay structure for Boza:

- scenes and persistent roots
- named game-object handles
- spatial transforms
- staged ECS system declarations

## Module Synopsis

| Item | Value |
| --- | --- |
| Primary import | `import boza.ecs;` |
| Core handles | `Scene`, `GameObject`, `Transform` |
| System DSL | `With`, `Opt`, `Without`, `RunAfter`, `RunBefore`, `Phase`, and stage aliases |

## Reference

### `Scene`

```cpp
class Scene final;
```

#### Static and Global Surface

| Member | Purpose |
| --- | --- |
| `Scene::create(name, active)` | create a scene root |
| `Scene::get(name)` | look up a scene by name |
| `Scene::main` | global property for the active main scene |
| `Scene::persistent` | global property for the persistent scene |

#### Instance Surface

| Member | Purpose |
| --- | --- |
| `destroy()` | mark the scene for destruction |
| `root()` | access the root game object |
| `name` | scene name property |
| `active` | active state property |
| `valid()` | validity check |

Example:

```cpp
Scene::main = Scene::create("Gameplay");
Scene ui = Scene::create("PauseMenu", false);
ui.active = true;
```

### `GameObject`

```cpp
class GameObject;
```

#### Creation and Lookup

| Member | Purpose |
| --- | --- |
| `GameObject::create(name, scene, active)` | create under a scene |
| `GameObject::create(name, parent, active)` | create under another object |
| `GameObject::create(name, active)` | create under `Scene::main` |
| `GameObject::find(name)` | find in `Scene::main` |
| `find_child(name)` | find a direct child |

#### Hierarchy and Identity

| Member | Purpose |
| --- | --- |
| `name` | object name property |
| `parent` | parent property |
| `active` | effective active state |
| `active_self` | local active state |
| `children()` | materialized child list |
| `for_each_child(callback)` | iterate children |
| `valid()` | validity check |

#### Component Surface

| Member | Purpose |
| --- | --- |
| `add_component<C>(...)` | add a component |
| `ensure_component<C>(...)` | add if missing, otherwise reuse |
| `get_component<C>()` | fetch an existing component |
| `try_get_component<C>()` | nullable component lookup |
| `has_component<C>()` | component presence check |
| `remove_component<C>()` | remove a component |

#### Relation Surface

| Member | Purpose |
| --- | --- |
| `add_relation<R>(target, ...)` | add relation data |
| `ensure_relation<R>(target, ...)` | ensure relation data exists |
| `get_relation_data<R>(target)` | fetch relation data |
| `try_get_relation_data<R>(target)` | nullable relation lookup |
| `has_relation<R>(target)` | relation presence check |
| `remove_relation<R>(target)` | remove relation |
| `get_relation_target<R>()` | fetch the related target |
| `for_each_with_relation<R>(callback)` | iterate matching relations |

Example:

```cpp
GameObject player = GameObject::create("Player");

auto& transform = player.get_component<Transform>();
transform.local_position = glm::vec3{ 0.0f, 1.0f, 0.0f };

player.add_component<PlayerController>();
```

### `Transform`

```cpp
class Transform final;
```

#### World-Space Properties

- `position`
- `rotation`
- `scale`
- `eulers`

#### Local-Space Properties

- `local_position`
- `local_rotation`
- `local_scale`
- `local_eulers`

#### Direction Properties

- `forward`
- `right`
- `up`

#### Methods

| Member | Purpose |
| --- | --- |
| `world_matrix()` | world transform matrix |
| `local_matrix()` | local transform matrix |
| `view_matrix()` | inverse world matrix, useful for cameras |
| `look_at(target, world_up)` | orient toward a target |

Example:

```cpp
auto& transform = obj.get_component<Transform>();
transform.local_position = glm::vec3{ 0.0f, 3.0f, 0.0f };
transform.local_scale = glm::vec3{ 2.0f };
transform.look_at(glm::vec3{ 0.0f, 0.0f, 0.0f });
```

### Systems DSL

Core spec helpers:

| Helper | Purpose |
| --- | --- |
| `With<T>` | required component or tag |
| `Opt<T>` | optional component |
| `Without<T>` | exclusion filter |
| `RunAfter<T>` | explicit stage ordering |
| `RunBefore<T>` | explicit stage ordering |
| `Phase` | lifecycle/update phase enum |

#### Stage Aliases

- `EngineBeginStage`
- `StartStage`
- `PostStartStage`
- `EnginePhysicsStage`
- `PhysicsStage`
- `EngineUpdateStage`
- `PreUpdateStage`
- `UpdateStage`
- `PostUpdateStage`
- `PreRenderStage`
- `EngineRenderStage`
- `DestroyStage`
- `EngineDestroyStage`
- `StandaloneStage`

Example:

```cpp
struct Spinner
{
    float speed{ 1.0f };
};

struct SpinnerSystem : UpdateStage<SpinnerSystem, With<Transform>, With<const Spinner>>
{
    static void execute(Transform& transform, const Spinner& spinner)
    {
        transform.local_eulers =
            transform.local_eulers + glm::vec3{ 0.0f, spinner.speed * Time::delta_time(), 0.0f };
    }
};
```

## Contracts

- `GameObject::destroy()` is deferred. The object is marked for destruction and disabled immediately, then removed later by the engine.
- Created objects are expected to work with `Transform` as part of the normal workflow.
- Plain structs make good custom component types.
- If an input callback needs major ECS structural changes, queue that work for a later system instead of doing everything immediately inside the callback.

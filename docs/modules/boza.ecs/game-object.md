# GameObject

[<- Previous](scene.md) | [Up](README.md) | [Next ->](transform.md)

Module: `boza.ecs`

Declared in: `boza/public/ecs/core/game_object.ixx`

## Contents

- [Synopsis](#synopsis)
- [Creation and Lookup](#creation-and-lookup)
- [Hierarchy and Identity](#hierarchy-and-identity)
- [Component Surface](#component-surface)
- [Relation Surface](#relation-surface)
- [Example](#example)

## Synopsis

```cpp
class GameObject;
```

## Creation and Lookup

| Member | Purpose |
| --- | --- |
| `GameObject::create(name, scene, active)` | create under a scene |
| `GameObject::create(name, parent, active)` | create under another object |
| `GameObject::create(name, active)` | create under `Scene::main` |
| `GameObject::find(name)` | find in `Scene::main` |
| `find_child(name)` | find a direct child |

## Hierarchy and Identity

| Member | Purpose |
| --- | --- |
| `name` | object name property |
| `parent` | parent property |
| `active` | effective active state |
| `active_self` | local active state |
| `children()` | materialized child list |
| `for_each_child(callback)` | iterate children |
| `valid()` | validity check |
| `destroy()` | deferred destruction request |

## Component Surface

| Member | Purpose |
| --- | --- |
| `add_component<C>(...)` | add a component |
| `ensure_component<C>(...)` | add if missing, otherwise reuse |
| `get_component<C>()` | fetch an existing component |
| `try_get_component<C>()` | nullable component lookup |
| `has_component<C>()` | component presence check |
| `remove_component<C>()` | remove a component |

## Relation Surface

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

## Example

```cpp
GameObject player = GameObject::create("Player");

auto& transform = player.get_component<Transform>();
transform.local_position = glm::vec3{ 0.0f, 1.0f, 0.0f };

player.add_component<PlayerController>();
```

[<- Previous](scene.md) | [Up](README.md) | [Next ->](transform.md)

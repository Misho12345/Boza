# Transform

[<- Previous](game-object.md) | [Up](README.md) | [Next ->](systems.md)

Module: `boza.ecs`

Declared in: `boza/public/ecs/component/builtin/transform.ixx`

## Contents

- [Synopsis](#synopsis)
- [World-Space Properties](#world-space-properties)
- [Local-Space Properties](#local-space-properties)
- [Direction Properties](#direction-properties)
- [Methods](#methods)
- [Example](#example)

## Synopsis

```cpp
class Transform final;
```

## World-Space Properties

- `position`
- `rotation`
- `scale`
- `eulers`

## Local-Space Properties

- `local_position`
- `local_rotation`
- `local_scale`
- `local_eulers`

## Direction Properties

- `forward`
- `right`
- `up`

## Methods

| Member | Purpose |
| --- | --- |
| `world_matrix()` | world transform matrix |
| `local_matrix()` | local transform matrix |
| `view_matrix()` | inverse world matrix, useful for cameras |
| `look_at(target, world_up)` | orient toward a target |

## Example

```cpp
auto& transform = obj.get_component<Transform>();
transform.local_position = glm::vec3{ 0.0f, 3.0f, 0.0f };
transform.local_scale = glm::vec3{ 2.0f };
transform.look_at(glm::vec3{ 0.0f, 0.0f, 0.0f });
```

[<- Previous](game-object.md) | [Up](README.md) | [Next ->](systems.md)

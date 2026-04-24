# Systems DSL

[<- Previous](transform.md) | [Up](README.md) | [Next ->](../boza.gfx/README.md)

Module: `boza.ecs`

Declared in:

- `boza/public/ecs/system/component_list.ixx`
- `boza/public/ecs/system/system_common.ixx`
- `boza/public/ecs/system/system_stage.ixx`

## Contents

- [Core Spec Helpers](#core-spec-helpers)
- [Stage Aliases](#stage-aliases)
- [Example](#example)

## Core Spec Helpers

| Helper | Purpose |
| --- | --- |
| `With<T>` | required component or tag |
| `Opt<T>` | optional component |
| `Without<T>` | exclusion filter |
| `RunAfter<T>` | explicit stage ordering |
| `RunBefore<T>` | explicit stage ordering |
| `Phase` | lifecycle / update phase enum |

## Stage Aliases

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

## Example

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

[<- Previous](transform.md) | [Up](README.md) | [Next ->](../boza.gfx/README.md)

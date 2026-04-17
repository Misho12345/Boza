# Compute

[<- Previous](mesh-renderer.md) | [Up](README.md) | [Next ->](../boza.input/README.md)

Module: `boza.gfx`

Declared in: `boza/public/gfx/compute_dispatcher.ixx`

## Contents

- [Dispatcher](#dispatcher)
- [Dispatch Group](#dispatch-group)
- [Example](#example)

## Dispatcher

### `ComputeDispatcher`

```cpp
class ComputeDispatcher;
```

| Member | Purpose |
| --- | --- |
| constructor with shader name | create a dispatcher for one compute shader |
| `set(name, value)` | bind POD data |
| `set(name, texture)` | bind a texture |
| `set(name, buffer)` | bind a buffer |
| `dispatch(...)` | dispatch by output dimensions |
| `dispatch_groups(...)` | dispatch by explicit workgroup counts |
| `wait()` | wait for completion |
| `status()`, `working()`, `finished()`, `failed()` | query execution state |
| `work_group_size()` | inspect reflected workgroup size |

## Dispatch Group

### `ComputeDispatchGroup`

```cpp
class ComputeDispatchGroup;
```

| Member | Purpose |
| --- | --- |
| `add(...)` / `add_groups(...)` | append compute steps |
| `clear()` | clear recorded steps |
| `record()` | record the dispatch list |
| `submit()` | submit recorded work |
| `wait()` | wait for completion |
| `run()` | record, submit, and wait |
| `status()`, `working()`, `finished()`, `failed()` | query execution state |

## Example

```cpp
ComputeDispatcher dispatcher{ "material_showcase/uv" };
dispatcher
    .set("outImage", texture)
    .dispatch(256, 256, 1)
    .wait();
```

[<- Previous](mesh-renderer.md) | [Up](README.md) | [Next ->](../boza.input/README.md)

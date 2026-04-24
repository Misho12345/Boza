# Scene

[<- Previous](README.md) | [Up](README.md) | [Next ->](game-object.md)

Module: `boza.ecs`

Declared in: `boza/public/ecs/core/scene.ixx`

## Contents

- [Synopsis](#synopsis)
- [Static and Global Surface](#static-and-global-surface)
- [Instance Surface](#instance-surface)
- [Example](#example)

## Synopsis

```cpp
class Scene final;
```

## Static and Global Surface

| Member | Purpose |
| --- | --- |
| `Scene::create(name, active)` | create a scene root |
| `Scene::get(name)` | look up a scene by name |
| `Scene::main` | global property for the active main scene |
| `Scene::persistent` | global property for the persistent scene |

## Instance Surface

| Member | Purpose |
| --- | --- |
| `destroy()` | mark the scene for destruction |
| `root()` | access the root game object |
| `name` | scene name property |
| `active` | active state property |
| `valid()` | validity check |

## Example

```cpp
Scene::main = Scene::create("Gameplay");
Scene ui = Scene::create("PauseMenu", false);
ui.active = true;
```

[<- Previous](README.md) | [Up](README.md) | [Next ->](game-object.md)

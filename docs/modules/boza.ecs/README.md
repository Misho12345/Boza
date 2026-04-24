# boza.ecs

[<- Previous](../boza.core/time.md) | [Up](../README.md) | [Next ->](scene.md)

Module: `boza.ecs`

Declared in: `boza/public/ecs/ecs.ixx`

## Overview

`boza.ecs` is the main gameplay module. It exposes scenes, game objects, transforms, and the public staged-system DSL.

> [!IMPORTANT]
> Set `Scene::main` before relying on the shorthand `GameObject::create(name)` or `GameObject::find(name)`.

## Contents

- [Main Areas](#main-areas)
- [Reference Pages](#reference-pages)

## Main Areas

| Area | Purpose |
| --- | --- |
| scenes | top-level grouping and persistent roots |
| game objects | hierarchy, activation, naming, components, relations |
| transforms | local and world spatial state |
| systems | staged ECS declaration model |

## Reference Pages

| Page | Covers |
| --- | --- |
| [`Scene`](scene.md) | scene creation, lookup, root access, main scene, persistent scene |
| [`GameObject`](game-object.md) | creation, lookup, hierarchy, activation, components, relations |
| [`Transform`](transform.md) | local/world properties, direction vectors, matrices, `look_at()` |
| [`Systems DSL`](systems.md) | `With`, `Opt`, `Without`, ordering helpers, phase aliases |

[<- Previous](../boza.core/time.md) | [Up](../README.md) | [Next ->](scene.md)

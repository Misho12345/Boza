# boza.app

[<- Previous](../README.md) | [Up](../README.md) | [Next ->](app.md)

Module: `boza.app`

Declared in: `boza/public/app/app.ixx`

## Overview

`boza.app` defines the application-facing entry point for Boza targets. In normal project usage, you export an `App` subclass from the target module and let `boza_add_executable(...)` generate the entry point that constructs the app and runs the lifecycle.

> [!IMPORTANT]
> For targets created with `boza_add_executable(...)`, users normally do not call `init()` or `run()` manually.

## Contents

- [Primary Type](#primary-type)
- [Reference Pages](#reference-pages)
- [Authoring Rules](#authoring-rules)

## Primary Type

| Type | Purpose |
| --- | --- |
| [`App`](app.md) | application base class and global runtime control surface |

## Reference Pages

| Page | Covers |
| --- | --- |
| [`App`](app.md) | lifecycle, `setup()`, target authoring expectations, fullscreen, cursor state, target FPS, and shutdown |

## Authoring Rules

1. Define `export module <target>;`
2. Export an app class whose name matches the target converted from `snake_case` to `PascalCase`
3. Use `boza_add_executable(<target>)` in that target's `CMakeLists.txt`

[<- Previous](../README.md) | [Up](../README.md) | [Next ->](app.md)

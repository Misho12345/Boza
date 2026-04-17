# boza.app

`boza.app` defines the application-facing entry point for Boza targets.

> [!IMPORTANT]
> For targets created with `boza_add_executable(...)`, export an `App` subclass from the target module. The generated entry point constructs that class and calls `init()` / `run()` automatically.

## Contents

- [Overview](#overview)
- [Module Synopsis](#module-synopsis)
- [Reference](#reference)
  - [`App`](#app)
  - [Runtime Controls](#runtime-controls)
- [Contracts](#contracts)
- [Example](#example)

## Overview

This module is responsible for:

- defining the app class that owns engine startup and shutdown
- exposing the `setup()` override where user code initializes gameplay state
- providing global runtime controls such as fullscreen, cursor mode, and target FPS

## Module Synopsis

| Item | Value |
| --- | --- |
| Primary import | `import boza.app;` |
| Common import | `import boza;` |
| Primary type | `App` |
| Main override | `setup()` |
| Bootstrap model | generated entry point via `boza_add_executable(...)` |

## Reference

### `App`

```cpp
class App
{
public:
    App();
    virtual ~App();

    bool init();
    void run() const;

    static void toggle_fullscreen();
    static void quit();

    static inline GlobalProperty<...> cursor_state;
    static inline GlobalProperty<...> target_fps;

protected:
    virtual void setup();
};
```

#### Lifecycle Surface

| Member | Kind | Purpose |
| --- | --- | --- |
| `App()` | constructor | creates the application object and installs it as the active engine instance |
| `~App()` | destructor | tears down the application instance |
| `init()` | method | initializes settings, windowing, rendering, and engine-begin stages, then calls `setup()` |
| `run() const` | method | enters the main engine loop |
| `setup()` | virtual override point | user-defined startup logic for scenes, objects, resources, and bindings |

#### User-Authored Responsibility

In normal project targets, users primarily interact with `App` by:

1. exporting a subclass
2. overriding `setup()`
3. letting the generated entry point handle the rest

### Runtime Controls

| API | Category | Purpose |
| --- | --- | --- |
| `App::toggle_fullscreen()` | static function | toggles the active window between windowed and fullscreen |
| `App::quit()` | static function | requests shutdown of the main loop |
| `App::cursor_state` | global property | reads or updates the active cursor mode |
| `App::target_fps` | global property | reads or updates the engine target framerate |

Example:

```cpp
App::target_fps = 144.0f;
App::cursor_state = CursorState::HiddenLocked;

App::toggle_fullscreen();
App::quit();
```

## Contracts

- Export one `App` subclass per executable target.
- The generated entry point expects the exported app class name to match the target converted from `snake_case` to `PascalCase`.
- `setup()` runs after engine, window, and render initialization.
- Most target code should not call `init()` or `run()` manually.
- `App::target_fps` controls the frame target, not the fixed physics step.

## Example

```cpp
export module hello_boza;

import boza;

using namespace boza;

export class HelloBoza : public App
{
protected:
    void setup() override
    {
        Scene::main = Scene::create("MainScene");
        Log::info("Boza initialized");
    }
};
```

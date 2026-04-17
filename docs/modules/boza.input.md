# boza.input

`boza.input` exposes the public input API: keys, cursor modes, polling helpers, combos, bindings, and callback-style capture.

> [!TIP]
> Use `Input` for continuous controls and `InputCapture` for object-owned callbacks and bindings.

## Contents

- [Overview](#overview)
- [Module Synopsis](#module-synopsis)
- [Reference](#reference)
  - [Key Model](#key-model)
  - [`Input`](#input)
  - [`InputCapture`](#inputcapture)
  - [`KeyState`](#keystate)
- [Contracts](#contracts)

## Overview

This module provides the public input layer for gameplay code:

- key and mouse enumerations
- input event categories
- cursor mode control
- polling helpers
- callback-based capture stored on gameplay objects

## Module Synopsis

| Item | Value |
| --- | --- |
| Primary import | `import boza.input;` |
| Key types | `Key`, `Action`, `CursorState`, `KeyCombo`, `KeyBinding`, `KeyState` |
| Polling API | `Input` |
| Callback API | `InputCapture` |

## Reference

### Key Model

#### Core Types

| Type | Purpose |
| --- | --- |
| `Key` | keyboard and mouse key/button enum |
| `Action` | input event category |
| `CursorState` | cursor mode enum |
| `KeyCombo` | a set of keys that must all be held |
| `KeyBinding` | one or more valid combos |
| `KeyState` | per-key pressed/held state |

#### Composition Operators

| Expression | Meaning |
| --- | --- |
| `Key::Ctrl & Key::S` | a key combo |
| `Key::Space | Key::Enter` | alternative accepted inputs |

Example:

```cpp
KeyCombo save_combo = Key::Ctrl & Key::S;
KeyBinding confirm = Key::Space | Key::Enter;
```

#### Cursor Modes

- `CursorState::Normal`
- `CursorState::Hidden`
- `CursorState::Locked`
- `CursorState::HiddenLocked`

### `Input`

```cpp
class Input final;
```

| Member | Purpose |
| --- | --- |
| `Input::is_pressed(key)` | edge-triggered pressed test |
| `Input::is_held(key)` | held-state test |

Example:

```cpp
if (Input::is_pressed(Key::Space))
{
    Log::info("Jump pressed");
}

if (Input::is_held(Key::W))
{
    move_forward();
}
```

### `InputCapture`

```cpp
class InputCapture final;
```

`InputCapture` stores callback bindings on a game object.

#### Common Binding Forms

| Member | Purpose |
| --- | --- |
| `on<Action::Press>(Key key, callback)` | bind a press callback |
| `on<Action::Release>(Key key, callback)` | bind a release callback |
| `on<Action::Hold>(Key key, callback)` | bind a hold callback |
| `on<Action::DoubleClick>(Key key, callback)` | bind a double-click callback |
| `on<Action::Press>(KeyCombo combo, callback)` | bind a combo |
| `on<Action::Press>(KeyBinding binding, callback)` | bind alternatives |
| `on<Action::MouseMove>(callback)` | bind mouse-move callback |
| `on<Action::MouseScroll>(callback)` | bind mouse-scroll callback |
| `on_mouse_move(callback)` / `on_mouse_scroll(callback)` | convenience wrappers |
| `clear()` | clear all stored bindings |

Example:

```cpp
auto& capture = player.add_component<InputCapture>();

capture.on<Action::Press>(Key::F11, &App::toggle_fullscreen);
capture.on<Action::Press>(Key::Ctrl & Key::S, [] { Log::info("Save requested"); });
capture.on<Action::Press>(Key::Space | Key::Enter, [] { Log::info("Confirm"); });

capture.on<Action::MouseMove>([](glm::vec2 delta)
{
    Log::debug("Mouse delta: {}", delta);
});
```

### `KeyState`

```cpp
class KeyState final;
```

| Member | Purpose |
| --- | --- |
| `is_pressed()` | pressed-state query |
| `is_held()` | held-state query |
| `set_pressed(...)` | update pressed state |
| `set_held(...)` | update held state |
| `last_press_time` | timestamp used for double-click tracking |

Related constant:

- `double_click_timeout`

## Contracts

- Polling with `Input` is a good fit for continuous movement and camera control.
- `InputCapture` is a good fit for high-level actions and bindings that should live on a gameplay object.
- For major ECS structural changes, it is safer to queue work into a later system instead of doing everything immediately inside an input callback.
- `InputSystem` is public because it participates in the staged ECS pipeline, but most gameplay code should treat it as engine-managed infrastructure.

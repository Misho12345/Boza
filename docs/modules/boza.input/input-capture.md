# InputCapture

[<- Previous](input.md) | [Up](README.md) | [Next ->](../README.md)

Module: `boza.input`

Declared in: `boza/public/input/input_capture.ixx`

## Contents

- [Synopsis](#synopsis)
- [Binding Surface](#binding-surface)
- [Usage Pattern](#usage-pattern)
- [Related Types](#related-types)
- [Contracts](#contracts)

## Synopsis

```cpp
class InputCapture final;
```

## Binding Surface

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

## Usage Pattern

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

## Related Types

| Type | Purpose |
| --- | --- |
| `BindingEvent` | stored binding/callback pair used internally by the capture object |
| `KeyState` | per-key pressed / held state tracking, exported from `boza.input:key_state` |
| `double_click_timeout` | timeout constant used by double-click tracking |

## Contracts

- `InputCapture` is best for high-level actions and bindings that should live on a gameplay object.
- For major ECS structural changes, it is safer to queue work into a later system instead of doing everything immediately inside an input callback.

[<- Previous](input.md) | [Up](README.md) | [Next ->](../README.md)

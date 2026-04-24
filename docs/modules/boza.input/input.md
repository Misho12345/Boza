# Input

[<- Previous](keys-and-bindings.md) | [Up](README.md) | [Next ->](input-capture.md)

Module: `boza.input`

Declared in: `boza/public/input/input_system.ixx`

## Contents

- [Synopsis](#synopsis)
- [Polling Surface](#polling-surface)
- [Example](#example)

## Synopsis

```cpp
class Input final;
```

## Polling Surface

| Member | Purpose |
| --- | --- |
| `Input::is_pressed(key)` | edge-triggered pressed test |
| `Input::is_held(key)` | held-state test |

## Example

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

[<- Previous](keys-and-bindings.md) | [Up](README.md) | [Next ->](input-capture.md)

# Keys and Bindings

[<- Previous](README.md) | [Up](README.md) | [Next ->](input.md)

Module: `boza.input`

Declared in:

- `boza/public/input/keys.ixx`
- `boza/public/input/cursor_state.ixx`

## Contents

- [Core Types](#core-types)
- [Composition Operators](#composition-operators)
- [Cursor States](#cursor-states)

## Core Types

| Type | Purpose |
| --- | --- |
| `Key` | keyboard and mouse key/button enum |
| `Action` | input event category |
| `CursorState` | cursor mode enum |
| `KeyCombo` | a set of keys that must all be held |
| `KeyBinding` | one or more valid combos |

## Composition Operators

| Expression | Meaning |
| --- | --- |
| `Key::Ctrl & Key::S` | a key combo |
| `Key::Space | Key::Enter` | alternative accepted inputs |

Example:

```cpp
KeyCombo save_combo = Key::Ctrl & Key::S;
KeyBinding confirm = Key::Space | Key::Enter;
```

## Cursor States

- `CursorState::Normal`
- `CursorState::Hidden`
- `CursorState::Locked`
- `CursorState::HiddenLocked`

[<- Previous](README.md) | [Up](README.md) | [Next ->](input.md)

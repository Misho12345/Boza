# boza.input

[<- Previous](../boza.gfx/compute.md) | [Up](../README.md) | [Next ->](keys-and-bindings.md)

Module: `boza.input`

Declared in: `boza/public/input/input.ixx`

## Overview

`boza.input` exposes the public input API: keys, cursor modes, polling helpers, combos, bindings, and callback-style capture.

> [!TIP]
> Use `Input` for continuous controls and `InputCapture` for object-owned callbacks and bindings.

## Contents

- [Main Areas](#main-areas)
- [Reference Pages](#reference-pages)

## Main Areas

| Area | Purpose |
| --- | --- |
| key model | keys, actions, combos, bindings, cursor states |
| polling | direct per-frame key queries |
| capture | callback bindings stored on gameplay objects |

## Reference Pages

| Page | Covers |
| --- | --- |
| [`Keys and Bindings`](keys-and-bindings.md) | `Key`, `Action`, `CursorState`, `KeyCombo`, `KeyBinding`, operators |
| [`Input`](input.md) | `Input::is_pressed()` and `Input::is_held()` |
| [`InputCapture`](input-capture.md) | callback binding surface, mouse input, convenience helpers, usage patterns |

[<- Previous](../boza.gfx/compute.md) | [Up](../README.md) | [Next ->](keys-and-bindings.md)

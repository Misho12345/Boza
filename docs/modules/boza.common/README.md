# boza.common

[<- Previous](../boza.app/app.md) | [Up](../README.md) | [Next ->](flags.md)

Module: `boza.common`

Declared in: `boza/public/common/common.ixx`

## Overview

`boza.common` provides the shared utility layer reused throughout the public API. It is where Boza exposes convenience aliases, enum flags, the property model, container aliases, and GLM / JSON re-exports.

## Contents

- [Main Areas](#main-areas)
- [Reference Pages](#reference-pages)

## Main Areas

| Area | Purpose |
| --- | --- |
| flags | strongly typed enum-bitflag support |
| property model | field-like API backed by getter/setter methods |
| aliases | container aliases and namespace conveniences |
| math / serialization conveniences | GLM and JSON re-exports |

## Reference Pages

| Page | Covers |
| --- | --- |
| [`Flags<T>`](flags.md) | enum-flag operations and usage |
| [`Property` and `GlobalProperty`](properties.md) | method-backed property model, overload selection, and authoring patterns |
| [`Aliases, GLM, and JSON`](aliases-and-glm.md) | `fs`, `json`, literals, container aliases, GLM formatters |

[<- Previous](../boza.app/app.md) | [Up](../README.md) | [Next ->](flags.md)

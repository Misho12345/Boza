# Boza Documentation

This directory contains the documentation for the Boza public engine surface.

## Contents

- [Documentation Map](#documentation-map)
- [Reading Paths](#reading-paths)
- [Directory Tree](#directory-tree)

## Documentation Map

| Page | Purpose |
| --- | --- |
| [`public-api.md`](public-api.md) | public API landing page and task-based navigation |
| [`modules/README.md`](modules/README.md) | per-module reference index |

## Reading Paths

### If you are starting a new Boza target

1. [`public-api.md`](public-api.md)
2. [`modules/boza.app/README.md`](modules/boza.app/README.md)
3. [`modules/boza.ecs/README.md`](modules/boza.ecs/README.md)

### If you are writing gameplay logic

1. [`modules/boza.ecs/README.md`](modules/boza.ecs/README.md)
2. [`modules/boza.input/README.md`](modules/boza.input/README.md)
3. [`modules/boza.core/README.md`](modules/boza.core/README.md)

### If you are working on rendering or compute

1. [`modules/boza.gfx/README.md`](modules/boza.gfx/README.md)
2. [`modules/boza.common/README.md`](modules/boza.common/README.md)

## Directory Tree

```text
docs/
|- README.md
|- public-api.md
\- modules/
   |- README.md
   |- boza.app/
   |- boza.common/
   |- boza.core/
   |- boza.ecs/
   |- boza.gfx/
   \- boza.input/
```

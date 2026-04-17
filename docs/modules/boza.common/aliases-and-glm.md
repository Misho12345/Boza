# Aliases, GLM, and JSON

[<- Previous](properties.md) | [Up](README.md) | [Next ->](../boza.core/README.md)

Module: `boza.common`

Declared in:

- `boza/public/common/common.ixx`
- `boza/public/common/glm.ixx`
- `boza/public/common/gtl.ixx`

## Contents

- [Namespace Conveniences](#namespace-conveniences)
- [Container Aliases](#container-aliases)
- [GLM Support](#glm-support)

## Namespace Conveniences

| Export | Purpose |
| --- | --- |
| `fs` | alias for `std::filesystem` |
| `json` | alias for `nlohmann::json` |
| string literals | `std::string_literals` re-export |
| string-view literals | `std::string_view_literals` re-export |
| chrono literals | `std::chrono_literals` re-export |
| complex literals | `std::complex_literals` re-export |

## Container Aliases

| Alias group | Members |
| --- | --- |
| single-threaded | `flat_map`, `flat_set`, `node_map`, `node_set` |
| parallel | `mt::flat_map`, `mt::flat_set`, `mt::node_map`, `mt::node_set` |

## GLM Support

`boza.common:glm` re-exports GLM and adds `std::formatter` support for vectors, quaternions, and matrices.

Example:

```cpp
glm::vec3 position{ 1.0f, 2.0f, 3.0f };
Log::info("Position: {}", position);
```

[<- Previous](properties.md) | [Up](README.md) | [Next ->](../boza.core/README.md)

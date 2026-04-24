# Material

[<- Previous](mesh.md) | [Up](README.md) | [Next ->](texture.md)

Module: `boza.gfx`

Declared in: `boza/public/gfx/material.ixx`

## Contents

- [Synopsis](#synopsis)
- [MaterialSettings](#materialsettings)
- [Registry Surface](#registry-surface)
- [Binding Surface](#binding-surface)
- [Reflection and Introspection](#reflection-and-introspection)
- [Example](#example)

## Synopsis

```cpp
struct MaterialSettings;
class PropertyBinder;
class Material;
```

## MaterialSettings

`MaterialSettings` is the public description used when creating a material.

| Field | Purpose |
| --- | --- |
| `vertex_shader` | vertex shader name |
| `fragment_shader` | fragment shader name |
| `depth_compare_op` | depth compare mode |
| `depth_test_enable` | enable depth testing |
| `depth_write_enable` | enable depth writes |
| `cull_mode` | face culling mode |
| `front_face` | front-face winding |

## Registry Surface

| Member | Purpose |
| --- | --- |
| `Material::create(name, settings)` | create a named material |
| `Material::get(name)` / `try_get(name)` | lookup by name |
| `Material::exists(ptr)` | validity test for a material pointer |
| `Material::destroy(name)` | destroy by name |

## Binding Surface

| Member | Purpose |
| --- | --- |
| `material["binding_name"] = value` | reflected property assignment |
| `update_property(name, value)` | update reflected POD-like value data |
| `update_texture(name, texture)` | bind a texture |
| `update_texture(name, texture, sampler)` | bind a texture and sampler |
| `update_buffer(name, buffer)` | bind a buffer |
| `push_constants(name, value)` | write push constant data |
| `bind() const` | bind the material to the current command buffer |

## Reflection and Introspection

| Member | Purpose |
| --- | --- |
| `lookup_binding(name)` | inspect reflected binding metadata |
| `descriptor_set_count()` | number of descriptor sets used |
| `name()` | material name |
| `cpu_cull_enabled()` / `set_cpu_cull_enabled(...)` | CPU culling hint used by rendering code |

## Example

```cpp
Material& material = Material::create("my_material", {
    .vertex_shader = "default",
    .fragment_shader = "default",
    .cull_mode = CullMode::Back
});

material["material.albedo_color"] = glm::vec4{ 0.9f, 0.2f, 0.2f, 1.0f };
material["material.properties"] = glm::vec4{ 0.0f, 0.6f, 0.0f, 0.0f };
```

[<- Previous](mesh.md) | [Up](README.md) | [Next ->](texture.md)

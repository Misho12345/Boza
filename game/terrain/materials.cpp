module game.terrain;

import :materials;
import :config;

namespace game::terrain
{
    void ensure_surface_material()
    {
        auto& material = Material::create(
            surface_material_name,
            {
                .vertex_shader = "default",
                .fragment_shader = "default"
            });

        material["albedo_map"] = Texture::get_or_load("default.png");
        material["material.albedo_color"] = glm::vec4{ 0.47f, 0.56f, 0.43f, 1.0f };
        material["material.properties"] = glm::vec4{ 0.1f, 0.9f, 0.2f, 0.0f };
    }

    void ensure_chunk_bounds_material()
    {
        auto& material = Material::create(
            chunk_bounds_material_name,
            {
                .vertex_shader = "material_showcase/unlit",
                .fragment_shader = "material_showcase/unlit",
                .cull_mode = CullMode::None
            });

        material["albedo_map"] = Texture::get_or_load("default.png");
        material["material.albedo_color"] = glm::vec4{ 0.95f, 0.12f, 0.12f, 1.0f };
        material["material.properties"] = glm::vec4{ 0.0f, 1.0f, 0.0f, 0.0f };
    }
}

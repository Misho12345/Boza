export module material_showcase_2;

import std;
import boza;

using namespace boza;

namespace
{
    enum class SurfaceMapping : std::uint8_t
    {
        Uv,
        Triplanar,
        WorldBox,
    };

    constexpr float surface_mapping_value(const SurfaceMapping mapping)
    {
        switch (mapping)
        {
        case SurfaceMapping::Triplanar: return 1.0f;
        case SurfaceMapping::WorldBox: return 2.0f;
        case SurfaceMapping::Uv:
        default:
            return 0.0f;
        }
    }

    struct SurfaceControls final
    {
        float metallic_bias{ 0.0f };
        float roughness_scale{ 1.0f };
        float uv_scale{ 1.0f };
        float displacement_scale{ 0.0f };
        float tessellation_scale{ 1.0f };
        SurfaceMapping mapping{ SurfaceMapping::Uv };
        bool heightfield_displacement{ false };
    };

    struct Showcase2CameraController
    {
        float move_speed{ 42.0f };
        float sensitivity{ 0.0015f };
        float yaw{ 0.0f };
        float pitch{ 0.0f };
    };

    struct Showcase2CameraControllerSystem
    {
        struct Start : StartStage<Start, With<Transform>, With<Showcase2CameraController>, With<InputCapture>>
        {
            static void execute(GameObject go, Transform& transform, Showcase2CameraController& controller, InputCapture& input)
            {
                const glm::vec3 forward = transform.forward();
                controller.yaw = std::atan2(forward.x, forward.z);
                controller.pitch = std::asin(glm::clamp(-forward.y, -1.0f, 1.0f));

                input.on<Action::MouseMove>([go](const glm::vec2 delta) mutable
                {
                    if (App::cursor_state != CursorState::HiddenLocked) return;

                    auto& controller = go.get_component<Showcase2CameraController>();
                    controller.yaw += delta.x * controller.sensitivity;
                    controller.pitch = glm::clamp(
                        controller.pitch + delta.y * controller.sensitivity,
                        glm::radians(-89.0f),
                        glm::radians(89.0f));
                });
            }
        };

        struct Update : UpdateStage<Update, With<Transform>, With<Showcase2CameraController>>
        {
            static void execute(Transform& transform, Showcase2CameraController& controller)
            {
                glm::vec3 move{ 0.0f };
                if (Input::is_held(Key::W)) move.z += 1.0f;
                if (Input::is_held(Key::S)) move.z -= 1.0f;
                if (Input::is_held(Key::A)) move.x -= 1.0f;
                if (Input::is_held(Key::D)) move.x += 1.0f;
                if (Input::is_held(Key::Space)) move.y += 1.0f;
                if (Input::is_held(Key::LShift)) move.y -= 1.0f;

                if (glm::length2(move) > 1e-8f)
                {
                    const glm::vec3 forward_xz = glm::normalize(glm::vec3{ transform.forward().x, 0.0f, transform.forward().z });
                    const glm::vec3 right_xz = glm::normalize(glm::vec3{ transform.right().x, 0.0f, transform.right().z });
                    glm::vec3 world_move = right_xz * move.x + forward_xz * move.z + glm::vec3{ 0.0f, move.y, 0.0f };
                    if (glm::length2(world_move) > 1e-8f)
                        transform.local_position = transform.local_position + glm::normalize(world_move) * controller.move_speed * Time::delta_time();
                }

                const glm::quat yaw_rotation = glm::angleAxis(controller.yaw, glm::vec3{ 0.0f, 1.0f, 0.0f });
                const glm::quat pitch_rotation = glm::angleAxis(controller.pitch, glm::vec3{ 1.0f, 0.0f, 0.0f });
                transform.local_rotation = glm::normalize(yaw_rotation * pitch_rotation);
            }
        };
    };

    void create_cube_mesh()
    {
        Mesh::create(
            "showcase2/cube",
            std::vector<Vertex>{
                { { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
                { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
                { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
                { { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
                { { 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
                { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
                { { -0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
                { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
                { { -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                { { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                { { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
                { { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
                { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },
                { { 0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
                { { 0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
                { { -0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
                { { 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                { { 0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
                { { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
                { { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                { { -0.5f, -0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                { { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
                { { -0.5f, 0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } }
            },
            std::vector<std::uint32_t>{
                0, 1, 2, 0, 2, 3,
                4, 5, 6, 4, 6, 7,
                8, 9, 10, 8, 10, 11,
                12, 13, 14, 12, 14, 15,
                16, 17, 18, 16, 18, 19,
                20, 21, 22, 20, 22, 23
            });
    }

    void create_pyramid_mesh()
    {
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;

        auto append_triangle = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                                   const glm::vec2& uv_a, const glm::vec2& uv_b, const glm::vec2& uv_c)
        {
            const glm::vec3 normal = glm::normalize(glm::cross(c - a, b - a));
            const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
            vertices.push_back(Vertex{ a, normal, uv_a });
            vertices.push_back(Vertex{ b, normal, uv_b });
            vertices.push_back(Vertex{ c, normal, uv_c });
            indices.insert(indices.end(), { base, base + 2u, base + 1u });
        };

        const glm::vec3 apex{ 0.0f, 0.5f, 0.0f };
        const glm::vec3 lbf{ -0.5f, -0.5f, 0.5f };
        const glm::vec3 rbf{ 0.5f, -0.5f, 0.5f };
        const glm::vec3 lbb{ -0.5f, -0.5f, -0.5f };
        const glm::vec3 rbb{ 0.5f, -0.5f, -0.5f };

        append_triangle(lbf, rbf, apex, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f });
        append_triangle(rbb, lbb, apex, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f });
        append_triangle(lbb, lbf, apex, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f });
        append_triangle(rbf, rbb, apex, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f });

        append_triangle(lbb, rbb, rbf, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f });
        append_triangle(lbb, rbf, lbf, { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f });

        Mesh::create("showcase2/pyramid", std::move(vertices), std::move(indices));
    }

    void create_slope_mesh()
    {
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;

        auto append_triangle = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                                   const glm::vec2& uv_a, const glm::vec2& uv_b, const glm::vec2& uv_c,
                                   const bool reverse_winding = true)
        {
            const glm::vec3 normal = reverse_winding
                ? glm::normalize(glm::cross(c - a, b - a))
                : glm::normalize(glm::cross(b - a, c - a));
            const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
            vertices.push_back(Vertex{ a, normal, uv_a });
            vertices.push_back(Vertex{ b, normal, uv_b });
            vertices.push_back(Vertex{ c, normal, uv_c });
            if (reverse_winding) indices.insert(indices.end(), { base, base + 2u, base + 1u });
            else indices.insert(indices.end(), { base, base + 1u, base + 2u });
        };

        const glm::vec3 lbf{ -0.5f, -0.5f, 0.5f };
        const glm::vec3 rbf{ 0.5f, -0.5f, 0.5f };
        const glm::vec3 lbb{ -0.5f, -0.5f, -0.5f };
        const glm::vec3 rbb{ 0.5f, -0.5f, -0.5f };
        const glm::vec3 ltb{ -0.5f, 0.5f, -0.5f };
        const glm::vec3 rtb{ 0.5f, 0.5f, -0.5f };

        append_triangle(lbb, rbb, rbf, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f });
        append_triangle(lbb, rbf, lbf, { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f });

        append_triangle(rbb, lbb, ltb, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f });
        append_triangle(rbb, ltb, rtb, { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f });

        append_triangle(lbb, lbf, ltb, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f });
        append_triangle(rbf, rbb, rtb, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f });

        append_triangle(ltb, rtb, rbf, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, false);
        append_triangle(ltb, rbf, lbf, { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, false);

        Mesh::create("showcase2/slope", std::move(vertices), std::move(indices));
    }

    void create_icosphere_mesh()
    {
        constexpr float radius = 0.5f;
        const float phi = (1.0f + std::sqrt(5.0f)) * 0.5f;

        std::vector<glm::vec3> positions{
            glm::normalize(glm::vec3{ -1.0f, phi, 0.0f }),
            glm::normalize(glm::vec3{ 1.0f, phi, 0.0f }),
            glm::normalize(glm::vec3{ -1.0f, -phi, 0.0f }),
            glm::normalize(glm::vec3{ 1.0f, -phi, 0.0f }),
            glm::normalize(glm::vec3{ 0.0f, -1.0f, phi }),
            glm::normalize(glm::vec3{ 0.0f, 1.0f, phi }),
            glm::normalize(glm::vec3{ 0.0f, -1.0f, -phi }),
            glm::normalize(glm::vec3{ 0.0f, 1.0f, -phi }),
            glm::normalize(glm::vec3{ phi, 0.0f, -1.0f }),
            glm::normalize(glm::vec3{ phi, 0.0f, 1.0f }),
            glm::normalize(glm::vec3{ -phi, 0.0f, -1.0f }),
            glm::normalize(glm::vec3{ -phi, 0.0f, 1.0f })
        };

        std::vector<glm::uvec3> faces{
            { 0u, 11u, 5u }, { 0u, 5u, 1u }, { 0u, 1u, 7u }, { 0u, 7u, 10u }, { 0u, 10u, 11u },
            { 1u, 5u, 9u }, { 5u, 11u, 4u }, { 11u, 10u, 2u }, { 10u, 7u, 6u }, { 7u, 1u, 8u },
            { 3u, 9u, 4u }, { 3u, 4u, 2u }, { 3u, 2u, 6u }, { 3u, 6u, 8u }, { 3u, 8u, 9u },
            { 4u, 9u, 5u }, { 2u, 4u, 11u }, { 6u, 2u, 10u }, { 8u, 6u, 7u }, { 9u, 8u, 1u }
        };

        std::unordered_map<std::uint64_t, std::uint32_t> midpoint_cache;
        auto midpoint_index = [&](const std::uint32_t a, const std::uint32_t b)
        {
            const std::uint32_t min_index = std::min(a, b);
            const std::uint32_t max_index = std::max(a, b);
            const std::uint64_t key = (static_cast<std::uint64_t>(min_index) << 32) | max_index;

            if (const auto it = midpoint_cache.find(key); it != midpoint_cache.end()) return it->second;

            const glm::vec3 midpoint = glm::normalize((positions[a] + positions[b]) * 0.5f);
            const std::uint32_t index = static_cast<std::uint32_t>(positions.size());
            positions.push_back(midpoint);
            midpoint_cache.emplace(key, index);
            return index;
        };

        std::vector<glm::uvec3> subdivided_faces;
        subdivided_faces.reserve(faces.size() * 4u);

        for (const glm::uvec3& face : faces)
        {
            const std::uint32_t ab = midpoint_index(face.x, face.y);
            const std::uint32_t bc = midpoint_index(face.y, face.z);
            const std::uint32_t ca = midpoint_index(face.z, face.x);

            subdivided_faces.insert(subdivided_faces.end(), {
                glm::uvec3{ face.x, ab, ca },
                glm::uvec3{ face.y, bc, ab },
                glm::uvec3{ face.z, ca, bc },
                glm::uvec3{ ab, bc, ca }
            });
        }

        std::vector<Vertex> vertices;
        vertices.reserve(positions.size());
        for (const glm::vec3& unit_pos : positions)
        {
            const glm::vec3 normal = glm::normalize(unit_pos);
            const float u = 0.5f + std::atan2(normal.z, normal.x) / glm::two_pi<float>();
            const float v = 0.5f - std::asin(normal.y) / glm::pi<float>();
            vertices.push_back(Vertex{ unit_pos * radius, normal, glm::vec2{ u, v } });
        }

        std::vector<std::uint32_t> indices;
        indices.reserve(subdivided_faces.size() * 3u);
        for (const glm::uvec3& face : subdivided_faces)
            indices.insert(indices.end(), { face.x, face.y, face.z });

        Mesh::create("showcase2/icosphere", std::move(vertices), std::move(indices));
    }

    [[nodiscard]] float rect_distance_2d(const glm::vec2 p, const glm::vec2 center, const glm::vec2 half_extents)
    {
        const glm::vec2 delta = glm::abs(p - center) - half_extents;
        return glm::length(glm::max(delta, glm::vec2{ 0.0f })) + std::min(std::max(delta.x, delta.y), 0.0f);
    }

    [[nodiscard]] float smooth_region_mask(
        const glm::vec2 p,
        const glm::vec2 center,
        const glm::vec2 half_extents,
        const float feather)
    {
        return 1.0f - glm::smoothstep(0.0f, feather, rect_distance_2d(p, center, half_extents));
    }

    float terrain_height(const float x, const float z)
    {
        const glm::vec2 point{ x, z };
        const float radial_distance = glm::length(point);
        const float outer_mask = glm::smoothstep(24.0f, 78.0f, radial_distance);

        float height =
            (std::sin(x * 0.032f) * 3.2f +
             std::cos(z * 0.028f) * 2.4f +
             std::sin((x - z) * 0.024f) * 1.6f +
             std::sin(radial_distance * 0.05f - 0.9f) * 2.1f) * outer_mask;

        const float inner_basin = -0.35f * (1.0f - glm::smoothstep(18.0f, 64.0f, radial_distance));
        height += inner_basin;

        float structure_mask = 0.0f;
        structure_mask = std::max(structure_mask, smooth_region_mask(point, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 72.0f, 56.0f }, 14.0f));
        structure_mask = std::max(structure_mask, smooth_region_mask(point, glm::vec2{ 0.0f, 52.0f }, glm::vec2{ 38.0f, 10.0f }, 8.0f));
        structure_mask = std::max(structure_mask, smooth_region_mask(point, glm::vec2{ 0.0f, -52.0f }, glm::vec2{ 38.0f, 10.0f }, 8.0f));
        structure_mask = std::max(structure_mask, smooth_region_mask(point, glm::vec2{ 58.0f, 6.0f }, glm::vec2{ 12.0f, 30.0f }, 10.0f));
        structure_mask = std::max(structure_mask, smooth_region_mask(point, glm::vec2{ -58.0f, 6.0f }, glm::vec2{ 12.0f, 30.0f }, 10.0f));

        const float sculpted_floor = 2.05f + 0.15f * std::sin(x * 0.04f) * std::cos(z * 0.04f);
        return std::lerp(height, sculpted_floor, glm::clamp(structure_mask, 0.0f, 1.0f));
    }

    glm::vec3 terrain_normal(const float x, const float z)
    {
        constexpr float delta = 0.35f;
        const float hx0 = terrain_height(x - delta, z);
        const float hx1 = terrain_height(x + delta, z);
        const float hz0 = terrain_height(x, z - delta);
        const float hz1 = terrain_height(x, z + delta);

        return glm::normalize(glm::vec3{ hx0 - hx1, 2.0f * delta, hz0 - hz1 });
    }

    void create_terrain_mesh()
    {
        constexpr std::uint32_t cells = 72;
        constexpr float extent = 180.0f;
        constexpr float half_extent = extent * 0.5f;
        constexpr float spacing = extent / static_cast<float>(cells);

        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;

        vertices.reserve((cells + 1u) * (cells + 1u));
        indices.reserve(cells * cells * 6u);

        for (std::uint32_t z = 0; z <= cells; ++z)
        {
            for (std::uint32_t x = 0; x <= cells; ++x)
            {
                const float world_x = -half_extent + static_cast<float>(x) * spacing;
                const float world_z = -half_extent + static_cast<float>(z) * spacing;
                vertices.push_back(Vertex{
                    .position = { world_x, terrain_height(world_x, world_z), world_z },
                    .normal = terrain_normal(world_x, world_z),
                    .tex_coord = {
                        static_cast<float>(x) / static_cast<float>(cells) * 6.0f,
                        static_cast<float>(z) / static_cast<float>(cells) * 6.0f
                    }
                });
            }
        }

        for (std::uint32_t z = 0; z < cells; ++z)
        {
            for (std::uint32_t x = 0; x < cells; ++x)
            {
                const std::uint32_t i0 = z * (cells + 1u) + x;
                const std::uint32_t i1 = i0 + 1u;
                const std::uint32_t i2 = i0 + (cells + 1u);
                const std::uint32_t i3 = i2 + 1u;

                indices.insert(indices.end(), { i0, i1, i2, i1, i3, i2 });
            }
        }

        Mesh::create("showcase2/terrain", std::move(vertices), std::move(indices));
    }

    void create_surface_material(
        const std::string& material_name,
        const std::string& folder,
        const std::string& roughness_map,
        const SurfaceControls& controls = {},
        const std::string& metallic_map = "")
    {
        Material& material = Material::create(material_name, {
            .vertex_shader = "default",
            .tess_control_shader = controls.displacement_scale > 0.0f ? "default" : "",
            .tess_evaluation_shader = controls.displacement_scale > 0.0f ? "default" : "",
            .fragment_shader = "default",
            .cull_mode = CullMode::Back
        });

        material["albedo_map"] = Texture::get_or_load(folder + "/diff.jpg");
        material["normal_map"] = Texture::get_or_load(folder + "/nor_gl.exr");
        material["roughness_map"] = Texture::get_or_load(roughness_map);
        if (controls.displacement_scale > 0.0f)
            material["height_map"] = Texture::get_or_load(folder + "/disp.png");

        if (!metallic_map.empty())
            material["metallic_map"] = Texture::get_or_load(metallic_map);

        material["material.albedo_color"] = glm::vec4{ 1.0f };
        material["material.properties"] = glm::vec4{
            controls.metallic_bias,
            controls.roughness_scale,
            controls.uv_scale,
            0.0f };
        material["material.detail"] = glm::vec4{
            controls.displacement_scale,
            surface_mapping_value(controls.mapping),
            controls.heightfield_displacement ? 1.0f : 0.0f,
            controls.tessellation_scale };
    }

    void create_materials()
    {
        create_surface_material("showcase2/brick_floor", "brick_floor", "brick_floor/rough.jpg", {
            .roughness_scale = 0.9f,
            .uv_scale = 0.38f,
            .tessellation_scale = 1.0f,
            .mapping = SurfaceMapping::WorldBox });
        create_surface_material("showcase2/castle_brick", "castle_brick_02_red", "castle_brick_02_red/rough.jpg", {
            .roughness_scale = 0.95f,
            .uv_scale = 0.34f,
            .displacement_scale = 0.065f,
            .tessellation_scale = 1.4f,
            .mapping = SurfaceMapping::WorldBox });
        create_surface_material("showcase2/damaged_plaster", "damaged_plaster", "damaged_plaster/rough.exr", {
            .roughness_scale = 0.85f,
            .uv_scale = 0.28f,
            .displacement_scale = 0.035f,
            .tessellation_scale = 1.2f,
            .mapping = SurfaceMapping::WorldBox });
        create_surface_material("showcase2/herringbone_parquet", "herringbone_parquet", "herringbone_parquet/rough.exr", {
            .roughness_scale = 0.65f,
            .uv_scale = 0.42f,
            .tessellation_scale = 1.0f,
            .mapping = SurfaceMapping::WorldBox });
        create_surface_material("showcase2/interior_tiles", "interior_tiles", "interior_tiles/rough.exr", {
            .roughness_scale = 0.55f,
            .uv_scale = 0.34f,
            .tessellation_scale = 1.0f,
            .mapping = SurfaceMapping::WorldBox });
        create_surface_material("showcase2/plaster_stone", "plaster_stone_wall_02", "plaster_stone_wall_02/rough.exr", {
            .roughness_scale = 0.9f,
            .uv_scale = 0.26f,
            .displacement_scale = 0.072f,
            .tessellation_scale = 1.5f,
            .mapping = SurfaceMapping::WorldBox });
        create_surface_material("showcase2/rocky_terrain", "rocky_terrain_02", "rocky_terrain_02/rough.exr", {
            .roughness_scale = 1.0f,
            .uv_scale = 0.1f,
            .displacement_scale = 0.18f,
            .tessellation_scale = 2.6f,
            .mapping = SurfaceMapping::Triplanar,
            .heightfield_displacement = true });
        create_surface_material("showcase2/rusty_metal_03", "rusty_metal_03", "rusty_metal_03/rough.exr", {
            .metallic_bias = 0.55f,
            .roughness_scale = 0.7f,
            .uv_scale = 0.34f,
            .displacement_scale = 0.006f,
            .tessellation_scale = 0.8f,
            .mapping = SurfaceMapping::WorldBox });
        create_surface_material(
            "showcase2/rusty_metal_04",
            "rusty_metal_04",
            "rusty_metal_04/rough.exr",
            {
                .roughness_scale = 0.65f,
                .uv_scale = 0.34f,
                .displacement_scale = 0.004f,
                .tessellation_scale = 0.75f,
                .mapping = SurfaceMapping::WorldBox },
            "rusty_metal_04/metal.exr");
        create_surface_material("showcase2/rusty_metal_grid", "rusty_metal_grid", "rusty_metal_grid/rough.exr", {
            .metallic_bias = 0.85f,
            .roughness_scale = 0.55f,
            .uv_scale = 0.44f,
            .displacement_scale = 0.008f,
            .tessellation_scale = 0.85f,
            .mapping = SurfaceMapping::WorldBox });
        create_surface_material("showcase2/dirt", "dirt", "dirt/rough.exr", {
            .roughness_scale = 1.0f,
            .uv_scale = 0.11f,
            .displacement_scale = 0.028f,
            .tessellation_scale = 1.2f,
            .mapping = SurfaceMapping::Triplanar });
    }

    [[nodiscard]] glm::vec3 resolved_mesh_piece_scale(
        const std::string_view mesh_name,
        const glm::vec3& scale)
    {
        if (mesh_name == "showcase2/icosphere")
        {
            const float uniform_scale = std::max({ scale.x, scale.y, scale.z });
            return glm::vec3{ uniform_scale };
        }

        return scale;
    }

    [[nodiscard]] float resting_center_y(
        const std::string_view mesh_name,
        const glm::vec3& scale,
        const float support_top_y,
        const float clearance = 0.0f)
    {
        return support_top_y + resolved_mesh_piece_scale(mesh_name, scale).y * 0.5f + clearance;
    }

    GameObject create_block(
        std::string_view name,
        const glm::vec3& position,
        const glm::vec3& scale,
        std::string_view material_name)
    {
        auto object = GameObject::create(name);
        auto& transform = object.get_component<Transform>();
        transform.local_position = position;
        transform.local_scale = scale;

        auto& renderer = object.add_component<MeshRenderer>();
        renderer.mesh_name = "showcase2/cube";
        renderer.material_name = material_name;
        object.add_component<ShadowCaster>();
        return object;
    }

    GameObject create_mesh_piece(
        std::string_view name,
        std::string_view mesh_name,
        const glm::vec3& position,
        const glm::vec3& scale,
        std::string_view material_name,
        const glm::quat& rotation = glm::identity<glm::quat>())
    {
        auto object = GameObject::create(name);
        auto& transform = object.get_component<Transform>();
        const glm::vec3 resolved_scale = resolved_mesh_piece_scale(mesh_name, scale);
        transform.local_position = position;
        transform.local_scale = resolved_scale;
        transform.local_rotation = rotation;

        auto& renderer = object.add_component<MeshRenderer>();
        renderer.mesh_name = mesh_name;
        renderer.material_name = material_name;
        object.add_component<ShadowCaster>();
        return object;
    }

    void build_staircase(
        std::string_view base_name,
        const glm::vec3& start,
        const glm::vec3& step_offset,
        const std::uint32_t step_count,
        std::string_view material_name)
    {
        for (std::uint32_t i = 0; i < step_count; ++i)
        {
            const float t = static_cast<float>(i);
            create_block(
                std::format("{}_step_{}", base_name, i),
                start + step_offset * t,
                glm::vec3{ 4.4f, 0.8f + t * 0.35f, 2.8f },
                material_name);
        }
    }

    void build_arch(std::string_view base_name, const glm::vec3& center, std::string_view material_name)
    {
        create_block(std::format("{}_left", base_name), center + glm::vec3{ -4.0f, 5.0f, 0.0f }, glm::vec3{ 2.5f, 10.0f, 2.5f }, material_name);
        create_block(std::format("{}_right", base_name), center + glm::vec3{ 4.0f, 5.0f, 0.0f }, glm::vec3{ 2.5f, 10.0f, 2.5f }, material_name);
        create_block(std::format("{}_cap", base_name), center + glm::vec3{ 0.0f, 10.0f, 0.0f }, glm::vec3{ 10.0f, 2.5f, 2.5f }, material_name);
    }

    void build_colonnade(
        std::string_view base_name,
        const glm::vec3& start,
        const glm::vec3& step,
        const std::uint32_t count,
        std::string_view column_material,
        std::string_view beam_material)
    {
        if (count == 0) return;

        glm::vec3 beam_scale{ 3.2f, 1.6f, 3.2f };
        if (std::abs(step.x) >= std::abs(step.z))
            beam_scale.x = std::max(std::abs(step.x), 3.2f);
        else
            beam_scale.z = std::max(std::abs(step.z), 3.2f);

        for (std::uint32_t i = 0; i < count; ++i)
        {
            const glm::vec3 ground_center = start + step * static_cast<float>(i);
            create_block(
                std::format("{}_pedestal_{}", base_name, i),
                ground_center + glm::vec3{ 0.0f, 0.8f, 0.0f },
                glm::vec3{ 4.2f, 1.6f, 4.2f },
                beam_material);
            create_block(
                std::format("{}_column_{}", base_name, i),
                ground_center + glm::vec3{ 0.0f, 6.8f, 0.0f },
                glm::vec3{ 2.5f, 10.4f, 2.5f },
                column_material);
            create_block(
                std::format("{}_capital_{}", base_name, i),
                ground_center + glm::vec3{ 0.0f, 12.4f, 0.0f },
                glm::vec3{ 3.4f, 0.8f, 3.4f },
                beam_material);

            if (i + 1u >= count) continue;

            create_block(
                std::format("{}_beam_{}", base_name, i),
                ground_center + step * 0.5f + glm::vec3{ 0.0f, 13.2f, 0.0f },
                beam_scale,
                beam_material);
        }
    }

    void build_pavilion(
        std::string_view base_name,
        const glm::vec3& ground_center,
        std::string_view floor_material,
        std::string_view column_material,
        std::string_view roof_material)
    {
        create_block(
            std::format("{}_floor", base_name),
            ground_center + glm::vec3{ 0.0f, 1.0f, 0.0f },
            glm::vec3{ 18.0f, 2.0f, 14.0f },
            floor_material);

        constexpr std::array<glm::vec2, 4> corner_offsets{
            glm::vec2{ -6.2f, -4.2f },
            glm::vec2{ 6.2f, -4.2f },
            glm::vec2{ -6.2f, 4.2f },
            glm::vec2{ 6.2f, 4.2f }
        };

        for (std::size_t i = 0; i < corner_offsets.size(); ++i)
        {
            const glm::vec3 corner_center{
                ground_center.x + corner_offsets[i].x,
                ground_center.y,
                ground_center.z + corner_offsets[i].y
            };

            create_block(
                std::format("{}_footing_{}", base_name, i),
                corner_center + glm::vec3{ 0.0f, 1.0f, 0.0f },
                glm::vec3{ 3.6f, 2.0f, 3.6f },
                floor_material);
            create_block(
                std::format("{}_column_{}", base_name, i),
                corner_center + glm::vec3{ 0.0f, 7.0f, 0.0f },
                glm::vec3{ 2.4f, 10.0f, 2.4f },
                column_material);
        }

        create_block(
            std::format("{}_roof", base_name),
            ground_center + glm::vec3{ 0.0f, 13.0f, 0.0f },
            glm::vec3{ 19.0f, 2.0f, 15.0f },
            roof_material);
        create_block(
            std::format("{}_roof_cap", base_name),
            ground_center + glm::vec3{ 0.0f, 14.4f, 0.0f },
            glm::vec3{ 11.0f, 0.8f, 7.0f },
            roof_material);
    }

    void build_tower(
        std::string_view base_name,
        const glm::vec3& ground_center,
        std::string_view material_name)
    {
        create_block(
            std::format("{}_base", base_name),
            ground_center + glm::vec3{ 0.0f, 2.2f, 0.0f },
            glm::vec3{ 8.0f, 4.4f, 8.0f },
            material_name);
        create_block(
            std::format("{}_shaft", base_name),
            ground_center + glm::vec3{ 0.0f, 10.2f, 0.0f },
            glm::vec3{ 5.4f, 11.6f, 5.4f },
            material_name);
        create_block(
            std::format("{}_cap", base_name),
            ground_center + glm::vec3{ 0.0f, 17.2f, 0.0f },
            glm::vec3{ 7.2f, 2.4f, 7.2f },
            material_name);
    }

    void setup_camera()
    {
        auto camera_object = GameObject::create("Showcase2Camera");
        auto& transform = camera_object.get_component<Transform>();
        transform.local_position = glm::vec3{ -92.0f, 46.0f, 102.0f };
        transform.look_at(glm::vec3{ 0.0f, 10.0f, 4.0f });

        auto& camera = camera_object.add_component<Camera>();
        camera.fov = 68.0f;
        camera.near_clip = 0.05f;
        camera.far_clip = 420.0f;
        camera.is_primary = true;

        camera_object.add_component<Showcase2CameraController>();
        camera_object.add_component<InputCapture>();
    }

    void setup_input()
    {
        auto& input = Scene::main().root().add_component<InputCapture>();
        input.on<Action::Press>(Key::F11, &App::toggle_fullscreen);
        input.on<Action::Press>(Key::MouseLeft, [] { App::cursor_state = CursorState::HiddenLocked; });
        input.on<Action::Press>(Key::Esc, []
        {
            if (App::cursor_state != CursorState::Normal) App::cursor_state = CursorState::Normal;
            else App::quit();
        });
    }

    void setup_lights()
    {
        auto sun = GameObject::create("Showcase2Sun");
        sun.get_component<Transform>().look_at(glm::vec3{ -0.42f, -0.90f, -0.18f });

        auto& directional = sun.add_component<DirectionalLight>();
        directional.color = glm::vec3{ 1.0f, 0.94f, 0.86f };
        directional.intensity = 1.7f;
        directional.casts_shadows = true;
        directional.shadow_strength = 0.76f;

        constexpr std::array accent_colors{
            glm::vec3{ 1.0f, 0.45f, 0.22f },
            glm::vec3{ 0.22f, 0.55f, 1.0f },
            glm::vec3{ 0.45f, 1.0f, 0.35f },
            glm::vec3{ 1.0f, 0.84f, 0.32f },
            glm::vec3{ 0.92f, 0.36f, 0.84f },
            glm::vec3{ 0.34f, 1.0f, 0.92f },
            glm::vec3{ 1.0f, 0.62f, 0.74f },
            glm::vec3{ 0.72f, 1.0f, 0.46f }
        };

        constexpr std::array light_positions{
            glm::vec3{ -34.0f, 10.5f, 28.0f },
            glm::vec3{ -12.0f, 9.5f, 28.0f },
            glm::vec3{ 12.0f, 9.5f, 28.0f },
            glm::vec3{ 34.0f, 10.5f, 28.0f },
            glm::vec3{ -34.0f, 10.5f, -28.0f },
            glm::vec3{ -12.0f, 9.5f, -28.0f },
            glm::vec3{ 12.0f, 9.5f, -28.0f },
            glm::vec3{ 34.0f, 10.5f, -28.0f }
        };

        for (std::size_t i = 0; i < accent_colors.size(); ++i)
        {
            auto light = GameObject::create(std::format("Showcase2AccentLight_{}", i));
            light.get_component<Transform>().local_position = light_positions[i];

            auto& point = light.add_component<PointLight>();
            point.color = accent_colors[i];
            point.intensity = 3.8f;
            point.range = 52.0f;
            point.casts_shadows = i % 2 == 0;
            point.shadow_strength = 0.58f;

            auto& renderer = light.add_component<MeshRenderer>();
            renderer.mesh_name = "showcase2/cube";
            renderer.material_name = i % 2 == 0 ? "showcase2/rusty_metal_04" : "showcase2/rusty_metal_03";
            light.get_component<Transform>().local_scale = glm::vec3{ 0.65f };
        }

        constexpr std::array spotlight_positions{
            glm::vec3{ -24.0f, 26.0f, 24.0f },
            glm::vec3{ 24.0f, 26.0f, 24.0f },
            glm::vec3{ -24.0f, 26.0f, -24.0f },
            glm::vec3{ 24.0f, 26.0f, -24.0f },
            glm::vec3{ -50.0f, 24.0f, 18.0f },
            glm::vec3{ 50.0f, 24.0f, -18.0f }
        };

        constexpr std::array spotlight_targets{
            glm::vec3{ -8.0f, 7.0f, 8.0f },
            glm::vec3{ 8.0f, 7.0f, 8.0f },
            glm::vec3{ -8.0f, 7.0f, -8.0f },
            glm::vec3{ 8.0f, 7.0f, -8.0f },
            glm::vec3{ -46.0f, 8.0f, 22.0f },
            glm::vec3{ 46.0f, 8.0f, -22.0f }
        };

        constexpr std::array spotlight_colors{
            glm::vec3{ 1.0f, 0.90f, 0.72f },
            glm::vec3{ 0.74f, 0.86f, 1.0f },
            glm::vec3{ 0.98f, 0.82f, 0.56f },
            glm::vec3{ 0.66f, 0.84f, 1.0f },
            glm::vec3{ 1.0f, 0.78f, 0.48f },
            glm::vec3{ 0.70f, 0.92f, 1.0f }
        };

        for (std::size_t i = 0; i < spotlight_positions.size(); ++i)
        {
            auto light = GameObject::create(std::format("Showcase2SpotLight_{}", i));
            auto& transform = light.get_component<Transform>();
            transform.local_position = spotlight_positions[i];
            transform.look_at(spotlight_targets[i]);

            auto& spot = light.add_component<SpotLight>();
            spot.color = spotlight_colors[i];
            spot.intensity = 7.2f;
            spot.range = 78.0f;
            spot.inner_angle = glm::radians(18.0f);
            spot.outer_angle = glm::radians(30.0f);
            spot.casts_shadows = true;
            spot.shadow_strength = 0.62f;

            auto& renderer = light.add_component<MeshRenderer>();
            renderer.mesh_name = "showcase2/cube";
            renderer.material_name = "showcase2/rusty_metal_grid";
            transform.local_scale = glm::vec3{ 0.75f, 0.4f, 0.75f };
        }

        constexpr std::array gallery_light_positions{
            glm::vec3{ -54.0f, 18.0f, -4.0f },
            glm::vec3{ -54.0f, 18.0f, 16.0f },
            glm::vec3{ 54.0f, 18.0f, -4.0f },
            glm::vec3{ 54.0f, 18.0f, 16.0f },
            glm::vec3{ 0.0f, 18.0f, 52.0f },
            glm::vec3{ 0.0f, 18.0f, -52.0f }
        };

        constexpr std::array gallery_light_targets{
            glm::vec3{ -54.0f, 8.0f, -4.0f },
            glm::vec3{ -54.0f, 8.0f, 16.0f },
            glm::vec3{ 54.0f, 8.0f, -4.0f },
            glm::vec3{ 54.0f, 8.0f, 16.0f },
            glm::vec3{ 0.0f, 6.0f, 52.0f },
            glm::vec3{ 0.0f, 6.0f, -52.0f }
        };

        constexpr std::array gallery_light_colors{
            glm::vec3{ 1.0f, 0.88f, 0.72f },
            glm::vec3{ 0.76f, 0.88f, 1.0f },
            glm::vec3{ 1.0f, 0.82f, 0.62f },
            glm::vec3{ 0.78f, 1.0f, 0.86f },
            glm::vec3{ 1.0f, 0.80f, 0.54f },
            glm::vec3{ 0.72f, 0.90f, 1.0f }
        };

        for (std::size_t i = 0; i < gallery_light_positions.size(); ++i)
        {
            auto light = GameObject::create(std::format("Showcase2GalleryLight_{}", i));
            auto& transform = light.get_component<Transform>();
            transform.local_position = gallery_light_positions[i];
            transform.look_at(gallery_light_targets[i]);

            auto& spot = light.add_component<SpotLight>();
            spot.color = gallery_light_colors[i];
            spot.intensity = 8.8f;
            spot.range = 54.0f;
            spot.inner_angle = glm::radians(16.0f);
            spot.outer_angle = glm::radians(26.0f);
            spot.casts_shadows = true;
            spot.shadow_strength = 0.66f;

            auto& renderer = light.add_component<MeshRenderer>();
            renderer.mesh_name = "showcase2/cube";
            renderer.material_name = i % 2 == 0 ? "showcase2/rusty_metal_04" : "showcase2/rusty_metal_03";
            transform.local_scale = glm::vec3{ 0.6f, 0.36f, 0.6f };
        }
    }

    void setup_world()
    {
        const auto terrain_anchor = [](const float x, const float z, const float half_height)
        {
            return terrain_height(x, z) + half_height;
        };

        constexpr float inset_top = 4.4f;
        constexpr float terrace_top = 4.8f;

        auto terrain = GameObject::create("Showcase2Terrain");
        auto& terrain_renderer = terrain.add_component<MeshRenderer>();
        terrain_renderer.mesh_name = "showcase2/terrain";
        terrain_renderer.material_name = "showcase2/rocky_terrain";
        terrain.add_component<ShadowCaster>();

        create_block("CentralCourt", glm::vec3{ 0.0f, 3.0f, 0.0f }, glm::vec3{ 62.0f, 2.0f, 46.0f }, "showcase2/brick_floor");
        create_block("CentralCourtInset", glm::vec3{ 0.0f, 4.2f, 0.0f }, glm::vec3{ 52.0f, 0.4f, 38.0f }, "showcase2/interior_tiles");
        create_block("GrandWalkNorthSouth", glm::vec3{ 0.0f, 4.3f, 0.0f }, glm::vec3{ 14.0f, 0.6f, 56.0f }, "showcase2/herringbone_parquet");
        create_block("GrandWalkEastWest", glm::vec3{ 0.0f, 4.3f, 0.0f }, glm::vec3{ 42.0f, 0.6f, 10.0f }, "showcase2/herringbone_parquet");
        create_block("NorthTerrace", glm::vec3{ 0.0f, 4.4f, 36.0f }, glm::vec3{ 40.0f, 0.8f, 12.0f }, "showcase2/brick_floor");
        create_block("SouthTerrace", glm::vec3{ 0.0f, 4.4f, -36.0f }, glm::vec3{ 40.0f, 0.8f, 12.0f }, "showcase2/brick_floor");
        create_block("EastWingFloor", glm::vec3{ 46.0f, 4.4f, -20.0f }, glm::vec3{ 22.0f, 0.8f, 18.0f }, "showcase2/rusty_metal_grid");
        create_block("WestWingFloor", glm::vec3{ -46.0f, 4.4f, 20.0f }, glm::vec3{ 22.0f, 0.8f, 18.0f }, "showcase2/plaster_stone");

        create_block("NorthParapet", glm::vec3{ 0.0f, 5.2f, 22.0f }, glm::vec3{ 58.0f, 1.6f, 2.6f }, "showcase2/castle_brick");
        create_block("SouthParapet", glm::vec3{ 0.0f, 5.2f, -22.0f }, glm::vec3{ 58.0f, 1.6f, 2.6f }, "showcase2/plaster_stone");
        create_block("EastParapet", glm::vec3{ 30.0f, 5.2f, 0.0f }, glm::vec3{ 2.6f, 1.6f, 42.0f }, "showcase2/rusty_metal_grid");
        create_block("WestParapet", glm::vec3{ -30.0f, 5.2f, 0.0f }, glm::vec3{ 2.6f, 1.6f, 42.0f }, "showcase2/damaged_plaster");

        create_block("CentralDaisBase", glm::vec3{ 0.0f, 5.8f, 0.0f }, glm::vec3{ 18.0f, 3.6f, 14.0f }, "showcase2/plaster_stone");
        create_block("CentralDaisInset", glm::vec3{ 0.0f, 8.0f, 0.0f }, glm::vec3{ 12.0f, 0.8f, 8.0f }, "showcase2/rusty_metal_grid");
        create_block("CentralMonolith", glm::vec3{ 0.0f, 15.2f, 0.0f }, glm::vec3{ 4.4f, 13.6f, 4.4f }, "showcase2/plaster_stone");
        create_block("CentralMonolithCap", glm::vec3{ 0.0f, 22.6f, 0.0f }, glm::vec3{ 7.4f, 1.2f, 7.4f }, "showcase2/rusty_metal_03");

        build_arch("NorthGate", glm::vec3{ 0.0f, inset_top, 27.0f }, "showcase2/plaster_stone");
        build_arch("SouthGate", glm::vec3{ 0.0f, inset_top, -27.0f }, "showcase2/castle_brick");

        build_colonnade(
            "WestColonnade",
            glm::vec3{ -22.0f, inset_top, -18.0f },
            glm::vec3{ 0.0f, 0.0f, 12.0f },
            4u,
            "showcase2/damaged_plaster",
            "showcase2/plaster_stone");
        build_colonnade(
            "EastColonnade",
            glm::vec3{ 22.0f, inset_top, -18.0f },
            glm::vec3{ 0.0f, 0.0f, 12.0f },
            4u,
            "showcase2/castle_brick",
            "showcase2/plaster_stone");

        build_pavilion(
            "EastPavilion",
            glm::vec3{ 46.0f, terrace_top, -20.0f },
            "showcase2/brick_floor",
            "showcase2/rusty_metal_04",
            "showcase2/rusty_metal_grid");
        build_pavilion(
            "WestPavilion",
            glm::vec3{ -46.0f, terrace_top, 20.0f },
            "showcase2/herringbone_parquet",
            "showcase2/plaster_stone",
            "showcase2/castle_brick");

        constexpr std::array<std::string_view, 12> showcase_materials{
            "showcase2/castle_brick",
            "showcase2/plaster_stone",
            "showcase2/damaged_plaster",
            "showcase2/rusty_metal_grid",
            "showcase2/herringbone_parquet",
            "showcase2/interior_tiles",
            "showcase2/rusty_metal_03",
            "showcase2/rusty_metal_04",
            "showcase2/brick_floor",
            "showcase2/dirt",
            "showcase2/plaster_stone",
            "showcase2/castle_brick"
        };

        constexpr std::array<std::string_view, 12> showcase_meshes{
            "showcase2/pyramid",
            "showcase2/icosphere",
            "showcase2/slope",
            "showcase2/cube",
            "showcase2/icosphere",
            "showcase2/pyramid",
            "showcase2/slope",
            "showcase2/cube",
            "showcase2/icosphere",
            "showcase2/pyramid",
            "showcase2/slope",
            "showcase2/icosphere"
        };

        constexpr std::array showcase_x{ -24.0f, -8.0f, 8.0f, 24.0f };

        for (std::size_t i = 0; i < showcase_x.size(); ++i)
        {
            const float north_height = i % 2 == 0 ? 8.6f : 10.8f;
            const float south_height = i % 2 == 0 ? 9.8f : 7.8f;
            const float plinth_top_y = terrace_top + 3.6f;
            const glm::vec3 north_scale{
                4.0f,
                north_height * 0.64f,
                3.6f + static_cast<float>(i % 2) * 0.7f
            };
            const glm::vec3 south_scale{
                3.8f + static_cast<float>(i % 2) * 0.6f,
                south_height * 0.64f,
                3.8f
            };

            create_block(
                std::format("NorthPlinth_{}", i),
                glm::vec3{ showcase_x[i], terrace_top + 1.8f, 36.0f },
                glm::vec3{ 5.0f, 3.6f, 5.0f },
                "showcase2/plaster_stone");
            create_mesh_piece(
                std::format("NorthShowcase_{}", i),
                showcase_meshes[i],
                glm::vec3{ showcase_x[i], resting_center_y(showcase_meshes[i], north_scale, plinth_top_y, 0.2f), 36.0f },
                north_scale,
                showcase_materials[i],
                glm::angleAxis(glm::radians(12.0f + static_cast<float>(i) * 8.0f), glm::vec3{ 0.0f, 1.0f, 0.0f }));

            create_block(
                std::format("SouthPlinth_{}", i),
                glm::vec3{ showcase_x[i], terrace_top + 1.8f, -36.0f },
                glm::vec3{ 5.0f, 3.6f, 5.0f },
                "showcase2/castle_brick");
            create_mesh_piece(
                std::format("SouthShowcase_{}", i),
                showcase_meshes[i + 4u],
                glm::vec3{ showcase_x[i], resting_center_y(showcase_meshes[i + 4u], south_scale, plinth_top_y, 0.2f), -36.0f },
                south_scale,
                showcase_materials[i + 4u],
                glm::angleAxis(glm::radians(-18.0f - static_cast<float>(i) * 7.0f), glm::vec3{ 0.0f, 1.0f, 0.0f }));
        }

        constexpr std::array gallery_positions{
            glm::vec3{ -54.0f, terrace_top, -4.0f },
            glm::vec3{ -54.0f, terrace_top, 16.0f },
            glm::vec3{ 54.0f, terrace_top, -4.0f },
            glm::vec3{ 54.0f, terrace_top, 16.0f }
        };

        for (std::size_t i = 0; i < gallery_positions.size(); ++i)
        {
            const float pedestal_top_y = terrace_top + 3.6f;
            const glm::vec3 gallery_scale{ 4.8f, 7.2f + static_cast<float>(i % 2) * 1.4f, 4.8f };

            create_block(
                std::format("GalleryPedestal_{}", i),
                gallery_positions[i] + glm::vec3{ 0.0f, 1.8f, 0.0f },
                glm::vec3{ 5.4f, 3.6f, 5.4f },
                i % 2 == 0 ? "showcase2/plaster_stone" : "showcase2/castle_brick");
            create_mesh_piece(
                std::format("GalleryShowcase_{}", i),
                showcase_meshes[i + 8u],
                glm::vec3{ gallery_positions[i].x, resting_center_y(showcase_meshes[i + 8u], gallery_scale, pedestal_top_y, 0.2f), gallery_positions[i].z },
                gallery_scale,
                showcase_materials[i + 8u],
                glm::angleAxis(glm::radians(22.0f + static_cast<float>(i) * 14.0f), glm::vec3{ 0.0f, 1.0f, 0.0f }));
        }

        create_block(
            "NorthCauseway",
            glm::vec3{ 0.0f, terrain_anchor(0.0f, 52.0f, 0.5f), 52.0f },
            glm::vec3{ 34.0f, 1.0f, 8.0f },
            "showcase2/brick_floor");
        create_block(
            "SouthCauseway",
            glm::vec3{ 0.0f, terrain_anchor(0.0f, -52.0f, 0.5f), -52.0f },
            glm::vec3{ 34.0f, 1.0f, 8.0f },
            "showcase2/herringbone_parquet");
        create_block(
            "EastGalleryDeck",
            glm::vec3{ 58.0f, terrain_anchor(58.0f, 6.0f, 2.0f), 6.0f },
            glm::vec3{ 10.0f, 4.0f, 26.0f },
            "showcase2/rusty_metal_grid");
        create_block(
            "WestGalleryDeck",
            glm::vec3{ -58.0f, terrain_anchor(-58.0f, 6.0f, 2.0f), 6.0f },
            glm::vec3{ 10.0f, 4.0f, 26.0f },
            "showcase2/plaster_stone");

        build_staircase("NorthStairs", glm::vec3{ -18.0f, 4.8f, 20.0f }, glm::vec3{ 0.0f, 0.6f, 2.6f }, 5u, "showcase2/interior_tiles");
        build_staircase("SouthStairs", glm::vec3{ 18.0f, 4.8f, -20.0f }, glm::vec3{ 0.0f, 0.6f, -2.6f }, 5u, "showcase2/herringbone_parquet");

        create_mesh_piece(
            "NorthCausewaySphere",
            "showcase2/icosphere",
            glm::vec3{ 0.0f, resting_center_y("showcase2/icosphere", glm::vec3{ 5.2f }, terrain_height(0.0f, 52.0f) + 1.0f, 0.2f), 52.0f },
            glm::vec3{ 5.2f },
            "showcase2/rusty_metal_03");
        create_mesh_piece(
            "WestSlopeMarker",
            "showcase2/slope",
            glm::vec3{ -58.0f, terrain_anchor(-58.0f, -10.0f, 3.5f), -10.0f },
            glm::vec3{ 8.0f, 7.0f, 10.0f },
            "showcase2/damaged_plaster",
            glm::angleAxis(glm::radians(90.0f), glm::vec3{ 0.0f, 1.0f, 0.0f }));
        create_mesh_piece(
            "EastPyramidMarker",
            "showcase2/pyramid",
            glm::vec3{ 58.0f, terrain_anchor(58.0f, -10.0f, 4.5f), -10.0f },
            glm::vec3{ 7.2f, 9.0f, 7.2f },
            "showcase2/castle_brick",
            glm::angleAxis(glm::radians(20.0f), glm::vec3{ 0.0f, 1.0f, 0.0f }));

        build_tower("CourtTower_NW", glm::vec3{ -32.0f, inset_top, 24.0f }, "showcase2/castle_brick");
        build_tower("CourtTower_NE", glm::vec3{ 32.0f, inset_top, 24.0f }, "showcase2/plaster_stone");
        build_tower("CourtTower_SW", glm::vec3{ -32.0f, inset_top, -24.0f }, "showcase2/damaged_plaster");
        build_tower("CourtTower_SE", glm::vec3{ 32.0f, inset_top, -24.0f }, "showcase2/rusty_metal_grid");

        constexpr std::array terrain_mounds{
            glm::vec2{ -46.0f, 48.0f },
            glm::vec2{ 44.0f, 50.0f },
            glm::vec2{ -50.0f, -46.0f },
            glm::vec2{ 50.0f, -44.0f }
        };

        for (std::size_t i = 0; i < terrain_mounds.size(); ++i)
        {
            const float x = terrain_mounds[i].x;
            const float z = terrain_mounds[i].y;
            const float mound_height = 3.2f + static_cast<float>(i % 2) * 1.1f;
            create_block(
                std::format("TerrainMound_{}", i),
                glm::vec3{ x, terrain_anchor(x, z, mound_height * 0.5f), z },
                glm::vec3{ 12.0f - static_cast<float>(i % 2) * 2.0f, mound_height, 9.0f },
                i < 2 ? "showcase2/dirt" : "showcase2/damaged_plaster");
        }

        for (std::uint32_t i = 0; i < 5; ++i)
        {
            const float north_x = -40.0f + static_cast<float>(i) * 20.0f;
            const float south_x = -40.0f + static_cast<float>(i) * 20.0f;
            create_block(
                std::format("NorthWall_{}", i),
                glm::vec3{ north_x, terrain_anchor(north_x, 62.0f, 6.0f + static_cast<float>(i % 2)), 62.0f },
                glm::vec3{ 12.0f, 12.0f + static_cast<float>(i % 2) * 2.0f, 4.0f },
                i % 2 == 0 ? "showcase2/castle_brick" : "showcase2/plaster_stone");
            create_block(
                std::format("SouthWall_{}", i),
                glm::vec3{ south_x, terrain_anchor(south_x, -62.0f, 6.0f + static_cast<float>((i + 1u) % 2)), -62.0f },
                glm::vec3{ 12.0f, 12.0f + static_cast<float>((i + 1u) % 2) * 2.0f, 4.0f },
                i % 2 == 0 ? "showcase2/damaged_plaster" : "showcase2/rusty_metal_grid");
        }

        for (std::uint32_t i = 0; i < 6; ++i)
        {
            const float wall_z = -40.0f + static_cast<float>(i) * 16.0f;
            create_block(
                std::format("WestWall_{}", i),
                glm::vec3{ -68.0f, terrain_anchor(-68.0f, wall_z, 6.0f + static_cast<float>(i % 2)), wall_z },
                glm::vec3{ 4.0f, 12.0f + static_cast<float>(i % 2) * 2.0f, 11.0f },
                i % 2 == 0 ? "showcase2/damaged_plaster" : "showcase2/plaster_stone");
            create_block(
                std::format("EastWall_{}", i),
                glm::vec3{ 68.0f, terrain_anchor(68.0f, wall_z, 6.0f + static_cast<float>((i + 1u) % 2)), wall_z },
                glm::vec3{ 4.0f, 12.0f + static_cast<float>((i + 1u) % 2) * 2.0f, 11.0f },
                i % 2 == 0 ? "showcase2/castle_brick" : "showcase2/rusty_metal_grid");
        }

        build_tower("OuterTower_NW", glm::vec3{ -72.0f, terrain_height(-72.0f, 56.0f), 56.0f }, "showcase2/plaster_stone");
        build_tower("OuterTower_NE", glm::vec3{ 72.0f, terrain_height(72.0f, 56.0f), 56.0f }, "showcase2/castle_brick");
        build_tower("OuterTower_SW", glm::vec3{ -72.0f, terrain_height(-72.0f, -56.0f), -56.0f }, "showcase2/damaged_plaster");
        build_tower("OuterTower_SE", glm::vec3{ 72.0f, terrain_height(72.0f, -56.0f), -56.0f }, "showcase2/rusty_metal_grid");
    }
}

export class MaterialShowcase2 : public App
{
public:
    void setup() override
    {
        create_cube_mesh();
        create_pyramid_mesh();
        create_slope_mesh();
        create_icosphere_mesh();
        create_terrain_mesh();
        create_materials();

        Scene::main = Scene::create("MaterialShowcase2");

        setup_camera();
        setup_lights();
        setup_world();
        setup_input();
    }
};

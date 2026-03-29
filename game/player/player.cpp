module game.player;

import std;
import boza;
using namespace boza;

import :brush;
import :camera;
import :components;
import :editor;

namespace game::player
{
    GameObject create(const GameObject& terrain_world)
    {
        const glm::vec3 terrain_size = terrain::world_size(terrain_world);
        const glm::vec3 terrain_center = terrain::world_center(terrain_world);

        GameObject player = GameObject::create("Player");
        player.add_component<PlayerTag>();

        auto& transform = player.get_component<Transform>();
        transform.local_position = terrain_center + glm::vec3{
            -terrain_size.x * 0.75f,
            terrain_size.y * 0.75f,
            terrain_size.z * 0.75f
        };
        transform.look_at(terrain_center + glm::vec3{ 0.0f, terrain_size.y * 0.35f, 0.0f });

        auto& camera = player.add_component<Camera>();
        camera.fov = 70.0f;
        camera.near_clip = 0.1f;
        camera.far_clip = 1500.0f;
        camera.is_primary = true;

        auto& input_capture = player.add_component<InputCapture>();
        auto& camera_controller = player.add_component<CameraController>();
        auto& brush_controller = player.add_component<BrushController>();
        auto& terrain_editor = player.add_component<TerrainEditor>();

        brush_controller.terrain_world = terrain_world;
        brush_controller.max_offset = std::max({ terrain_size.x, terrain_size.y, terrain_size.z });
        brush_controller.radius = glm::clamp(
            brush_controller.radius,
            brush_controller.min_radius,
            terrain::max_brush_radius(terrain_world));

        terrain_editor.terrain_world = terrain_world;

        initialize_orientation(transform, camera_controller);

        brush_controller.cursor = GameObject::create(cursor_name, player);

        auto& cursor_transform = brush_controller.cursor.get_component<Transform>();
        cursor_transform.local_position = glm::vec3{ 0.0f, 0.0f, brush_controller.offset };
        cursor_transform.local_scale = glm::vec3{ brush_controller.radius * 2.0f };

        auto& cursor_renderer = brush_controller.cursor.add_component<MeshRenderer>();
        cursor_renderer.mesh_name = cursor_mesh_name;
        cursor_renderer.material_name = cursor_material_name;

        bind_camera_input(player, input_capture);
        bind_brush_input(player, input_capture);
        bind_editor_input(player, input_capture);

        return player;
    }
}

module game.player:editor;

import std;
import boza;
using namespace boza;

import game.terrain;

import :components;

namespace game::player
{
    void try_queue_edit(
        TerrainEditor& editor,
        BrushController& brush,
        const terrain::EditOperation operation,
        float& cooldown,
        const float interval)
    {
        if (cooldown > 0.0f) return;
        if (!brush.cursor.valid() || !editor.terrain_world.valid()) return;
        if (!terrain::ready(editor.terrain_world)) return;

        const auto* cursor_transform = brush.cursor.try_get_component<Transform>();
        if (!cursor_transform) return;

        const glm::vec3 cursor_world = cursor_transform->position();
        if (!terrain::intersects_world_sphere(editor.terrain_world, cursor_world, brush.radius)) return;

        terrain::queue_edit(
            editor.terrain_world,
            {
                .world_center = cursor_world,
                .radius = brush.radius,
                .operation = operation,
            });

        cooldown = interval;
    }

    void bind_editor_input(GameObject player, InputCapture& input_capture)
    {
        input_capture.on<Action::Hold>(Key::MouseLeft, [player]() mutable
        {
            auto* editor = player.try_get_component<TerrainEditor>();
            auto* brush = player.try_get_component<BrushController>();
            if (!editor || !brush) return;

            try_queue_edit(
                *editor,
                *brush,
                terrain::EditOperation::Remove,
                editor->remove_cooldown,
                editor->remove_interval);
        });

        input_capture.on<Action::Hold>(Key::MouseRight, [player]() mutable
        {
            auto* editor = player.try_get_component<TerrainEditor>();
            auto* brush = player.try_get_component<BrushController>();
            if (!editor || !brush) return;

            try_queue_edit(
                *editor,
                *brush,
                terrain::EditOperation::Add,
                editor->add_cooldown,
                editor->add_interval);
        });
    }

    struct TerrainEditorSystem final
    {
        struct Update final : UpdateStage<Update, With<TerrainEditor>>
        {
            static void execute(TerrainEditor& editor)
            {
                editor.remove_cooldown = std::max(0.0f, editor.remove_cooldown - Time::delta_time());
                editor.add_cooldown = std::max(0.0f, editor.add_cooldown - Time::delta_time());
            }
        };
    };
}

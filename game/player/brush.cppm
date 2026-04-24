module game.player:brush;

import game.terrain;

import :components;

namespace game::player
{
    void bind_brush_input(GameObject& player, InputCapture& input_capture)
    {
        input_capture.on<Action::Press>(Key::LCtrl & Key::B, [player]() mutable
        {
            auto* brush = player.try_get_component<BrushController>();
            if (!brush || !brush->terrain_world.valid()) return;

            const bool visible = !terrain::chunk_bounds_visible(brush->terrain_world);
            terrain::set_chunk_bounds_visible(brush->terrain_world, visible);
        });

        input_capture.on<Action::MouseScroll>([player](const glm::vec2 scroll) mutable
        {
            auto* brush = player.try_get_component<BrushController>();
            if (!brush || scroll.y == 0.0f) return;

            if (Input::is_held(Key::LCtrl))
            {
                brush->offset = glm::clamp(
                    brush->offset + scroll.y * brush->offset_step,
                    brush->min_offset,
                    brush->max_offset);
                return;
            }

            brush->radius = glm::clamp(
                brush->radius + scroll.y * brush->radius_step,
                brush->min_radius,
                terrain::max_brush_radius(brush->terrain_world));
        });
    }

    struct BrushControllerSystem final
    {
        struct Update final : UpdateStage<Update, With<BrushController>>
        {
            static void execute(BrushController& brush)
            {
                if (!brush.cursor.valid()) return;

                if (auto* cursor_transform = brush.cursor.try_get_component<Transform>())
                {
                    cursor_transform->local_position = glm::vec3{ 0.0f, 0.0f, brush.offset };
                    cursor_transform->local_scale = glm::vec3{ brush.radius * 2.0f };
                }
            }
        };
    };
}

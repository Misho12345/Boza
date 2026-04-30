export module boza.ecs:transform_system;

import :system_stage;
import :component_list;
import :tags;
import :transform;
import :game_object;

export namespace boza
{
    struct TransformSystem
    {
        struct Update
            : PreRenderStage<
                Update,
                With<const tags::TransformDirty>
            >
        {
            static void execute(GameObject go)
            {
                if (!go.valid()) return;
                if (!go.has_component<tags::TransformDirty>()) return;

                const GameObject parent = go.parent();
                if (parent.valid() &&
                    parent.has_component<tags::TransformDirty>()) return;

                update_subtree(go);
            }

        private:
            static void update_subtree(
                GameObject go,
                const bool parent_world_changed = true)
            {
                if (!go.valid()) return;

                auto& transform = go.get_component<Transform>();

                const bool should_evaluate = transform.dirty_ || parent_world_changed;
                if (should_evaluate) transform.evaluate_world_transform();

                if (should_evaluate) go.add_component<tags::RenderTransformDirty>();

                transform.dirty_ = false;

                go.remove_component<tags::TransformDirty>();
                for (const GameObject child : go.children())
                {
                    update_subtree(child, should_evaluate);
                }
            }
        };
    };
}

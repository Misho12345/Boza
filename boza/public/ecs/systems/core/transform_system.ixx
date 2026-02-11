export module boza.ecs:transform_system;

import :system_stage;
import :component_list;
import :tags;
import :game_object;

export namespace boza
{
    struct TransformSystem
    {
        struct Update : EngineUpdateStage<Update, With<const tags::TransformDirty>>
        {
            static void execute(GameObject go)
            {
                if (!go.valid()) return;
                if (!go.has_component<Transform>())
                {
                    go.remove_component<tags::TransformDirty>();
                    return;
                }

                const GameObject parent = go.parent();
                if (parent.valid() && parent.has_component<tags::TransformDirty>()) return;

                update_subtree(go, true);
            }

        private:
            static void update_subtree(GameObject go, const bool parent_world_changed)
            {
                if (!go.valid()) return;

                bool world_changed = parent_world_changed;

                if (go.has_component<Transform>())
                {
                    auto& transform = go.get_component<Transform>();

                    const bool should_evaluate = transform.dirty_ || parent_world_changed;
                    if (should_evaluate)
                    {
                        transform.evaluate_world_transform();
                    }

                    transform.dirty_ = false;
                    world_changed = should_evaluate;
                }

                go.remove_component<tags::TransformDirty>();
                go.for_each_child([world_changed](GameObject child) { update_subtree(child, world_changed); });
            }
        };
    };
}

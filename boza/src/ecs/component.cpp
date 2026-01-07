module boza.ecs;

import :component;
import :game_object;
import :scene;

namespace boza
{
    Transform&       Component::get_transform_ref() { return game_object_->transform(); }
    const Transform& Component::get_transform_cref() const { return game_object_->transform(); }

    Scene&       Component::get_scene_ref() { return game_object_->scene(); }
    const Scene& Component::get_scene_cref() const { return game_object_->scene(); }

    GameObject&       Component::get_game_object_ref() { return *game_object_; }
    const GameObject& Component::get_game_object_cref() const { return *game_object_; }

    void Component::set_enabled(const bool value)
    {
        if (enabled_ == value) return;

        enabled_ = value;

        game_object_->scene_->invalidate_caches();

        if (game_object_->active)
        {
            if (enabled_) on_enable();
            else on_disable();
        }
    }

    void Component::copy_base_component_data_to(Component* target) const
    {
        if (target) target->enabled_ = enabled_;
    }
}

module;

#include <cassert>

module boza.ecs;

namespace boza
{
    void Transform::look_at(const glm::vec3& target, const glm::vec3& world_up)
    {
        rotation = glm::quatLookAtLH(normalize(target - position_), world_up);
    }

    std::vector<Transform*> Transform::get_children() const
    {
        std::vector<Transform*> children;
        children.reserve(children_.size());
        for (const auto child_entity : children_)
        {
            children.push_back(&scene_->get_world().get<Transform>(child_entity));
        }
        return children;
    }

    void Transform::for_each_child(const std::function<void(Transform&)>& callback) const
    {
        for (const auto child_entity : children_)
        {
            callback(scene_->get_world().get<Transform>(child_entity));
        }
    }

    glm::vec3 Transform::get_position() const { return position_; }
    glm::quat Transform::get_rotation() const { return rotation_; }
    glm::vec3 Transform::get_scale() const { return scale_; }
    glm::vec3 Transform::get_eulers() const { return glm::eulerAngles(rotation_); }

    glm::vec3 Transform::get_local_position() const
    {
        if (!has_parent()) return position_;

        glm::vec3 rel = position_ - parent->position_;

        rel = inverse(parent->rotation_) * rel;
        rel = rel / parent->scale_;

        return rel;
    }

    glm::quat Transform::get_local_rotation() const
    {
        return has_parent() ? inverse(parent->rotation_) * rotation_ : rotation_;
    }

    glm::vec3 Transform::get_local_scale() const
    {
        return has_parent() ? scale_ / parent->scale_ : scale_;
    }

    glm::vec3 Transform::get_local_eulers() const { return glm::eulerAngles(get_local_rotation()); }

    void Transform::set_position(const glm::vec3& value)
    {
        for_each_child([&value, this](Transform& child) { child.position += value - position_; });

        position_ = value;
        mark_dirty();
    }

    void Transform::set_rotation(const glm::quat& value)
    {
        for_each_child([this, delta_rot = normalize(value * inverse(rotation_))](Transform& child)
        {
            child.position = position_ + glm::rotate(delta_rot, child.position_ - position_);
            child.rotation = delta_rot * child.rotation_;
        });

        rotation_ = value;
        mark_dirty();
    }

    void Transform::set_scale(const glm::vec3& value)
    {
        for_each_child([this, scale_factor = value / scale_](Transform& child)
        {
            child.position = position_ + scale_factor * (child.position_ - position_);
            child.scale    = scale_factor * child.scale_;
        });

        mark_dirty();
        scale_ = value;
    }

    void Transform::set_eulers(const glm::vec3& value) { set_rotation(glm::quat{ value }); }

    void Transform::set_local_position(const glm::vec3& value)
    {
        position = has_parent() ? glm::rotate(parent->rotation_, value * parent->scale_) + parent->position_ : value;
    }

    void Transform::set_local_rotation(const glm::quat& value)
    {
        rotation = has_parent() ? parent->rotation_ * value : value;
    }

    void Transform::set_local_scale(const glm::vec3& value)
    {
        scale = has_parent() ? parent->scale_ * value : value;
    }

    void Transform::set_local_eulers(const glm::vec3& value) { set_local_rotation(glm::quat{ value }); }

    glm::vec3 Transform::get_forward() const { return glm::rotate(rotation_, glm::vec3(0.0f, 0.0f, -1.0f)); }
    glm::vec3 Transform::get_right() const { return glm::rotate(rotation_, glm::vec3(1.0f, 0.0f, 0.0f)); }
    glm::vec3 Transform::get_up() const { return glm::rotate(rotation_, glm::vec3(0.0f, 1.0f, 0.0f)); }

    Transform& Transform::get_parent() const
    {
        auto* parent_transform = get_parent_transform();
        assert(parent_transform != nullptr && "Transform has no parent");
        return *parent_transform;
    }

    void Transform::set_parent(Transform& new_parent)
    {
        if (has_parent())
        {
            auto& registry = scene_->get_world();
            if (auto* old_parent = registry.try_get<Transform>(parent_))
            {
                std::erase(old_parent->children_, entity_);
            }
        }

        parent_ = new_parent.entity_;
        new_parent.children_.push_back(entity_);
        mark_dirty();
    }

    Transform* Transform::get_parent_transform() const
    {
        return has_parent() ? scene_->get_world().try_get<Transform>(parent_) : nullptr;
    }

    void Transform::mark_dirty() const
    {
        static auto mark_dirty_each = [](const Transform& t) { t.mark_dirty(); };

        is_dirty_ = true;
        for_each_child(mark_dirty_each);
    }

    void Transform::update_cached_matrix() const
    {
        const glm::mat4 trans = glm::translate(glm::mat4(1.0f), position_);
        const glm::mat4 rot   = glm::mat4_cast(rotation_);
        const glm::mat4 sc    = glm::scale(glm::mat4(1.0f), scale_);

        cached_model_matrix_ = trans * rot * sc;
        is_dirty_            = false;
    }

    glm::mat4 Transform::model_matrix() const
    {
        if (is_dirty_) update_cached_matrix();
        return cached_model_matrix_;
    }

    glm::mat4 Transform::view_matrix() const
    {
        const glm::mat4 rotation_matrix    = glm::mat4_cast(glm::conjugate(rotation_));
        const glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), -position_);
        return rotation_matrix * translation_matrix;
    }
}

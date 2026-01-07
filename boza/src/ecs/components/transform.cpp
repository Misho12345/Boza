module boza.ecs;

import boza.core;

namespace boza
{
    void Transform::look_at(const glm::vec3& target, const glm::vec3& world_up)
    {
        set_rotation(glm::quatLookAtLH(normalize(target - world_position_), world_up));
    }

    std::vector<Transform*> Transform::get_children() const { return children_; }

    void Transform::for_each_child(const std::function<void(Transform&)>& callback) const
    {
        for (auto* child : children_)
        {
            if (child) callback(*child);
        }
    }

    void Transform::set_position(const glm::vec3& value)
    {
        local_position_ = has_parent() ? glm::vec3(inverse(parent_->world_matrix_) * glm::vec4(value, 1.0f)) : value;
        mark_dirty();
    }

    void Transform::set_rotation(const glm::quat& value)
    {
        local_rotation_ = has_parent() ? inverse(parent_->world_rotation_) * value : value;
        mark_dirty();
    }

    void Transform::set_scale(const glm::vec3& value)
    {
        local_scale_ = has_parent() ? value / parent_->world_scale_ : value;
        mark_dirty();
    }

    void Transform::set_eulers(const glm::vec3& value)
    {
        set_rotation(glm::quat{ value });
    }

    void Transform::set_local_position(const glm::vec3& value)
    {
        local_position_ = value;
        mark_dirty();
    }

    void Transform::set_local_rotation(const glm::quat& value)
    {
        local_rotation_ = value;
        mark_dirty();
    }

    void Transform::set_local_scale(const glm::vec3& value)
    {
        local_scale_ = value;
        mark_dirty();
    }

    void Transform::set_local_eulers(const glm::vec3& value)
    {
        set_local_rotation(glm::quat{ value });
    }

    glm::vec3 Transform::get_forward() const { return glm::rotate(world_rotation_, glm::vec3{ 0.0f, 0.0f, 1.0f }); }
    glm::vec3 Transform::get_right() const { return glm::rotate(world_rotation_, glm::vec3{ 1.0f, 0.0f, 0.0f }); }
    glm::vec3 Transform::get_up() const { return glm::rotate(world_rotation_, glm::vec3{ 0.0f, 1.0f, 0.0f }); }

    void Transform::set_parent(Transform* new_parent, const ParentChangeStrategy strategy)
    {
        if (parent_ == new_parent) return;

        if (strategy == ParentChangeStrategy::KeepWorld)
        {
            const glm::vec3 old_world_pos = world_position_;
            const glm::quat old_world_rot = world_rotation_;
            const glm::vec3 old_world_scale = world_scale_;

            if (parent_)
            {
                auto& old_parent_children = parent_->children_;
                std::erase(old_parent_children, this);
            }

            parent_ = new_parent;

            if (new_parent)
            {
                new_parent->children_.push_back(this);

                const glm::mat4 parent_inverse = inverse(new_parent->world_matrix_);
                const glm::vec4 local_pos = parent_inverse * glm::vec4{ old_world_pos, 1.0f };
                local_position_ = glm::vec3{ local_pos };

                local_rotation_ = inverse(new_parent->world_rotation_) * old_world_rot;
                local_scale_ = old_world_scale / new_parent->world_scale_;
            }
            else
            {
                local_position_ = old_world_pos;
                local_rotation_ = old_world_rot;
                local_scale_ = old_world_scale;
            }
        }
        else
        {
            if (parent_)
            {
                auto& old_parent_children = parent_->children_;
                std::erase(old_parent_children, this);
            }

            parent_ = new_parent;

            if (new_parent) new_parent->children_.push_back(this);
        }

        mark_dirty();
    }

    void Transform::mark_dirty()
    {
        is_dirty_ = true;
        mark_children_dirty();
    }

    void Transform::mark_children_dirty() const
    {
        for (auto* child : children_)
        {
            if (child)
            {
                child->is_dirty_ = true;
                child->mark_children_dirty();
            }
        }
    }

    void Transform::evaluate_world_transform()
    {
        if (!is_dirty_) return;

        if (has_parent())
        {
            const glm::mat4 local_trans = glm::translate(glm::mat4(1.0f), local_position_);
            const glm::mat4 local_rot = glm::mat4_cast(local_rotation_);
            const glm::mat4 local_sc = glm::scale(glm::mat4(1.0f), local_scale_);
            const glm::mat4 local_mat = local_trans * local_rot * local_sc;

            world_matrix_ = parent_->world_matrix_ * local_mat;

            world_position_ = glm::vec3(world_matrix_[3]);
            world_rotation_ = parent_->world_rotation_ * local_rotation_;
            world_scale_ = parent_->world_scale_ * local_scale_;
        }
        else
        {
            world_position_ = local_position_;
            world_rotation_ = local_rotation_;
            world_scale_ = local_scale_;

            const glm::mat4 trans = glm::translate(glm::mat4(1.0f), world_position_);
            const glm::mat4 rot = glm::mat4_cast(world_rotation_);
            const glm::mat4 sc = glm::scale(glm::mat4(1.0f), world_scale_);
            world_matrix_ = trans * rot * sc;
        }

        is_dirty_ = false;
    }

    glm::mat4 Transform::local_matrix() const
    {
        const glm::mat4 trans = glm::translate(glm::mat4(1.0f), local_position_);
        const glm::mat4 rot = glm::mat4_cast(local_rotation_);
        const glm::mat4 sc = glm::scale(glm::mat4(1.0f), local_scale_);
        return trans * rot * sc;
    }

    glm::mat4 Transform::view_matrix() const
    {
        const glm::mat4 rotation_matrix = glm::mat4_cast(glm::conjugate(world_rotation_));
        const glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), -world_position_);
        return rotation_matrix * translation_matrix;
    }

    void Transform::on_clone(GameObject& target)
    {
        auto* cloned = target.try_get_component<Transform>();
        if (!cloned) return;

        copy_base_component_data_to(cloned);
        cloned->local_position_ = local_position_;
        cloned->local_rotation_ = local_rotation_;
        cloned->local_scale_ = local_scale_;
        cloned->mark_dirty();
    }
}

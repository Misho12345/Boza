module boza.ecs;

import std;
import <flecs.h>;
import :transform;
import boza.ecs;
import boza.core;

namespace boza
{
    [[nodiscard]]
    glm::quat normalize_or_identity(const glm::quat& q)
    {
        const float len2 = glm::dot(q, q);
        if (len2 <= std::numeric_limits<float>::epsilon()) return glm::identity<glm::quat>();
        return glm::normalize(q);
    }

    [[nodiscard]]
    glm::vec3 safe_divide(const glm::vec3& numerator, const glm::vec3& denominator)
    {
        constexpr float eps = std::numeric_limits<float>::epsilon();

        return {
            std::abs(denominator.x) > eps ? numerator.x / denominator.x : 0.0f,
            std::abs(denominator.y) > eps ? numerator.y / denominator.y : 0.0f,
            std::abs(denominator.z) > eps ? numerator.z / denominator.z : 0.0f
        };
    }

    [[nodiscard]]
    Transform* try_get_parent_transform(const flecs::entity entity)
    {
        if (!entity.is_valid()) return nullptr;

        const flecs::entity parent = entity.parent();
        if (!parent.is_valid() || !parent.has<Transform>()) return nullptr;

        return parent.try_get_mut<Transform>();
    }

    glm::vec3 Transform::get_position() const
    {
        ensure_world_transform_up_to_date();
        return world_position_;
    }

    glm::quat Transform::get_rotation() const
    {
        ensure_world_transform_up_to_date();
        return world_rotation_;
    }

    glm::vec3 Transform::get_scale() const
    {
        ensure_world_transform_up_to_date();
        return world_scale_;
    }

    glm::vec3 Transform::get_eulers() const
    {
        ensure_world_transform_up_to_date();
        return glm::eulerAngles(world_rotation_);
    }

    void Transform::ensure_world_transform_up_to_date() const
    {
        if (!entity_.is_valid()) return;

        auto* self = const_cast<Transform*>(this);
        const std::uint64_t current_frame = Time::frame_count();

        if (!self->dirty_ && self->last_ensure_frame_ == current_frame) return;

        if (!self->dirty_)
        {
            if (Transform* parent_t = try_get_parent_transform(entity_))
            {
                if (!parent_t->dirty_ &&
                    self->parent_world_revision_ == parent_t->world_revision_)
                {
                    self->last_ensure_frame_ = current_frame;
                    return;
                }
            }
            else if (self->parent_world_revision_ == 0)
            {
                self->last_ensure_frame_ = current_frame;
                return;
            }
        }

        bool parent_world_changed = false;
        if (Transform* parent_t = try_get_parent_transform(entity_))
        {
            parent_t->ensure_world_transform_up_to_date();
            parent_world_changed = self->parent_world_revision_ != parent_t->world_revision_;
        }
        else parent_world_changed = self->parent_world_revision_ != 0;

        if (self->dirty_ || parent_world_changed) self->evaluate_world_transform();

        self->last_ensure_frame_ = current_frame;
    }

    void Transform::mark_dirty() const
    {
        if (!entity_.is_valid()) return;

        auto* self = const_cast<Transform*>(this);
        self->dirty_ = true;
        (void)entity_.add<tags::TransformDirty>();
    }

    void Transform::set_position(const glm::vec3& value)
    {
        if (Transform* parent_t = try_get_parent_transform(entity_))
        {
            parent_t->ensure_world_transform_up_to_date();

            const glm::vec3 world_offset = value - parent_t->world_position_;
            const glm::vec3 parent_local_offset =
                glm::inverse(parent_t->world_rotation_) * world_offset;
            local_position_ = safe_divide(parent_local_offset, parent_t->world_scale_);
        }
        else local_position_ = value;

        mark_dirty();
    }

    void Transform::set_rotation(const glm::quat& value)
    {
        const glm::quat world_rotation = normalize_or_identity(value);

        if (Transform* parent_t = try_get_parent_transform(entity_))
        {
            parent_t->ensure_world_transform_up_to_date();
            local_rotation_ = normalize_or_identity(
                glm::inverse(parent_t->world_rotation_) * world_rotation);
        }
        else local_rotation_ = world_rotation;

        mark_dirty();
    }

    void Transform::set_scale(const glm::vec3& value)
    {
        if (Transform* parent_t = try_get_parent_transform(entity_))
        {
            parent_t->ensure_world_transform_up_to_date();
            local_scale_ = safe_divide(value, parent_t->world_scale_);
        }
        else local_scale_ = value;

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
        local_rotation_ = normalize_or_identity(value);
        mark_dirty();
    }

    void Transform::set_local_scale(const glm::vec3& value)
    {
        local_scale_ = value;
        mark_dirty();
    }

    void Transform::set_local_eulers(const glm::vec3& value)
    {
        local_rotation_ = normalize_or_identity(glm::quat{ value });
        mark_dirty();
    }

    glm::vec3 Transform::get_forward() const
    {
        ensure_world_transform_up_to_date();
        return glm::normalize(
            world_rotation_ * glm::vec3{ 0.0f, 0.0f, 1.0f });
    }

    glm::vec3 Transform::get_right() const
    {
        ensure_world_transform_up_to_date();
        return glm::normalize(
            world_rotation_ * glm::vec3{ 1.0f, 0.0f, 0.0f });
    }

    glm::vec3 Transform::get_up() const
    {
        ensure_world_transform_up_to_date();
        return glm::normalize(
            world_rotation_ * glm::vec3{ 0.0f, 1.0f, 0.0f });
    }

    void Transform::look_at(const glm::vec3& target, const glm::vec3& world_up)
    {
        ensure_world_transform_up_to_date();

        const glm::vec3 direction = target - world_position_;
        if (glm::length2(direction) <= std::numeric_limits<float>::epsilon()) return;

        set_rotation(glm::quatLookAt(glm::normalize(direction), world_up));
    }

    glm::mat4 Transform::local_matrix() const
    {
        const glm::mat4 trans = glm::translate(glm::mat4{ 1.0f }, local_position_);
        const glm::mat4 rot   = glm::mat4_cast(normalize_or_identity(local_rotation_));
        const glm::mat4 sc    = glm::scale(glm::mat4{ 1.0f }, local_scale_);
        return trans * rot * sc;
    }

    glm::mat4 Transform::world_matrix() const
    {
        ensure_world_transform_up_to_date();
        return world_matrix_;
    }

    glm::mat4 Transform::view_matrix() const
    {
        ensure_world_transform_up_to_date();
        return glm::inverse(world_matrix_);
    }

    void Transform::evaluate_world_transform()
    {
        local_rotation_ = normalize_or_identity(local_rotation_);

        if (Transform* parent_t = try_get_parent_transform(entity_))
        {
            parent_t->ensure_world_transform_up_to_date();

            world_position_ =
                parent_t->world_position_ +
                (parent_t->world_rotation_ *
                    (parent_t->world_scale_ * local_position_));
            world_rotation_ = normalize_or_identity(parent_t->world_rotation_ * local_rotation_);
            world_scale_ = parent_t->world_scale_ * local_scale_;
            parent_world_revision_ = parent_t->world_revision_;
        }
        else
        {
            world_position_ = local_position_;
            world_rotation_ = local_rotation_;
            world_scale_ = local_scale_;
            parent_world_revision_ = 0;
        }

        const glm::mat4 trans = glm::translate(glm::mat4{ 1.0f }, world_position_);
        const glm::mat4 rot   = glm::mat4_cast(world_rotation_);
        const glm::mat4 sc    = glm::scale(glm::mat4{ 1.0f }, world_scale_);
        world_matrix_ = trans * rot * sc;

        dirty_ = false;
        last_ensure_frame_ = Time::frame_count();
        ++world_revision_;
    }
}

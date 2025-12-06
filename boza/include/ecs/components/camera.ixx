module;

#include <cstddef>
#include "api.hpp"

export module boza.ecs:camera;

import boza.common;
import :component;

export namespace boza
{
    class BOZA_API Camera final : public Component
    {
    public:
        enum class ProjectionType { Perspective, Orthographic };

        Camera() = default;

        PropertyGetSet<Camera, ProjectionType> projection_type
        {
            &Camera::get_projection_type,
            &Camera::set_projection_type,
            offsetof(Camera, projection_type)
        };

        PropertyGetSet<Camera, float> fov
        {
            &Camera::get_fov,
            &Camera::set_fov,
            offsetof(Camera, fov)
        };

        PropertyGetSet<Camera, float> near_clip
        {
            &Camera::get_near_clip,
            &Camera::set_near_clip,
            offsetof(Camera, near_clip)
        };

        PropertyGetSet<Camera, float> far_clip
        {
            &Camera::get_far_clip,
            &Camera::set_far_clip,
            offsetof(Camera, far_clip)
        };

        PropertyGetSet<Camera, float> ortho_size
        {
            &Camera::get_ortho_size,
            &Camera::set_ortho_size,
            offsetof(Camera, ortho_size)
        };

        PropertyGetSet<Camera, bool> primary
        {
            &Camera::get_primary,
            &Camera::set_primary,
            offsetof(Camera, primary)
        };

        [[nodiscard]] glm::mat4 projection_matrix(float aspect_ratio) const;

    private:
        [[nodiscard]] ProjectionType get_projection_type() const { return projection_type_; }
        void set_projection_type(const ProjectionType value) { projection_type_ = value; }

        [[nodiscard]] float get_fov() const { return fov_; }
        void set_fov(const float value) { fov_ = value; }

        [[nodiscard]] float get_near_clip() const { return near_clip_; }
        void set_near_clip(const float value) { near_clip_ = value; }

        [[nodiscard]] float get_far_clip() const { return far_clip_; }
        void set_far_clip(const float value) { far_clip_ = value; }

        [[nodiscard]] float get_ortho_size() const { return ortho_size_; }
        void set_ortho_size(const float value) { ortho_size_ = value; }

        [[nodiscard]] bool get_primary() const { return primary_; }
        void set_primary(const bool value) { primary_ = value; }

        ProjectionType projection_type_{ ProjectionType::Perspective };
        float fov_{ 45.0f };
        float near_clip_{ 0.1f };
        float far_clip_{ 1000.0f };
        float ortho_size_{ 10.0f };
        bool primary_{ true };
    };
}
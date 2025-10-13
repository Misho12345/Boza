#pragma once
#include "boza/pch.hpp"
#include "Component.hpp"
#include "Property.hpp"

namespace boza
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

    class BOZA_API Transform final : public Component
    {
    public:
        PropertyGetSet<const glm::vec3&> position
        {
            GET -> const glm::vec3& { return position_; },
            SET(value) { update_position(value); }
        };

        PropertyGetSet<const glm::quat&> rotation
        {
            GET -> const glm::quat& { return rotation_; },
            SET(value) { update_rotation(value); }
        };

        PropertyGetSet<glm::vec3> eulers
        {
            GET { return glm::eulerAngles(rotation_); },
            SET(value) { update_rotation(glm::quat(value)); }
        };

        PropertyGetSet<const glm::vec3&> scale
        {
            GET -> const glm::vec3& { return scale_; },
            SET(value) { update_scale(value); }
        };

        PropertyGet<glm::mat4> model_matrix{ GET { return get_model_matrix(); } };
        PropertyGet<glm::mat4> view_matrix{ GET { return get_view_matrix(); } };

        PropertyGet<glm::vec3> forward{ GET { return get_forward(); } };
        PropertyGet<glm::vec3> right{ GET { return get_right(); } };
        PropertyGet<glm::vec3> up{ GET { return get_up(); } };

    private:
        void update_position(const glm::vec3& value);
        void update_rotation(const glm::quat& value);
        void update_scale(const glm::vec3& value);

        glm::mat4 get_model_matrix() const;
        glm::mat4 get_view_matrix() const;

        glm::vec3 get_forward() const;
        glm::vec3 get_right() const;
        glm::vec3 get_up() const;

        glm::vec3 position_{ 0.0f, 0.0f, 0.0f };
        glm::quat rotation_{ glm::identity<glm::quat>() };
        glm::vec3 scale_{ 1.0f, 1.0f, 1.0f };
    };

#ifdef _MSC_VER
#pragma warning(pop)
#endif
}

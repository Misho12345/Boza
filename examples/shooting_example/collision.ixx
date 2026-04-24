export module shooting_example:collision;

import std;
import boza;
using namespace boza;

import :components;

export namespace shooting_example
{
    struct OrientedBox final
    {
        glm::vec3 center{ 0.0f };
        glm::vec3 right{ 1.0f, 0.0f, 0.0f };
        glm::vec3 up{ 0.0f, 1.0f, 0.0f };
        glm::vec3 forward{ 0.0f, 0.0f, 1.0f };
        glm::vec3 extents{ 0.5f };
    };

    [[nodiscard]]
    inline OrientedBox make_oriented_box(const Transform& transform, const BoxCollider& collider)
    {
        const glm::vec3 position = transform.position;
        const glm::vec3 scale    = glm::abs(glm::vec3{ transform.scale });

        return {
            .center   = position,
            .right    = normalize(glm::vec3{ transform.right }),
            .up       = normalize(glm::vec3{ transform.up }),
            .forward  = normalize(glm::vec3{ transform.forward }),
            .extents  = collider.half_extents * scale
        };
    }

    [[nodiscard]]
    inline glm::vec3 localize_point(const OrientedBox& box, const glm::vec3& point)
    {
        const glm::vec3 delta = point - box.center;

        return {
            dot(delta, box.right),
            dot(delta, box.up),
            dot(delta, box.forward)
        };
    }

    [[nodiscard]]
    inline glm::vec3 worldize_vector(const OrientedBox& box, const glm::vec3& local_vector)
    {
        return box.right * local_vector.x +
               box.up * local_vector.y +
               box.forward * local_vector.z;
    }

    [[nodiscard]]
    inline glm::vec3 closest_point_on_box(const OrientedBox& box, const glm::vec3& point)
    {
        const glm::vec3 local_point = localize_point(box, point);
        const glm::vec3 clamped     = glm::clamp(local_point, -box.extents, box.extents);
        return box.center + worldize_vector(box, clamped);
    }

    [[nodiscard]]
    inline bool sphere_hits_box(const glm::vec3& center, const float radius, const OrientedBox& box)
    {
        const glm::vec3 closest = closest_point_on_box(box, center);
        return glm::length2(center - closest) <= radius * radius;
    }

    [[nodiscard]]
    inline bool sphere_hits_box(
        const glm::vec3&     center,
        const float          radius,
        const Transform&     transform,
        const BoxCollider&   collider)
    {
        return sphere_hits_box(center, radius, make_oriented_box(transform, collider));
    }

    [[nodiscard]]
    inline std::optional<glm::vec3> sphere_push_out(
        const glm::vec3& center,
        const float      radius,
        const OrientedBox& box)
    {
        const glm::vec3 local_center = localize_point(box, center);
        const glm::vec3 clamped      = glm::clamp(local_center, -box.extents, box.extents);
        const glm::vec3 delta_local  = local_center - clamped;
        const float     delta_len2   = glm::length2(delta_local);

        constexpr float epsilon = 1e-6f;

        if (delta_len2 > epsilon)
        {
            const float distance = std::sqrt(delta_len2);
            if (distance >= radius) return std::nullopt;

            const glm::vec3 normal_local = delta_local / distance;
            return worldize_vector(box, normal_local * (radius - distance));
        }

        const glm::vec3 distance_to_face = box.extents - glm::abs(local_center);

        int axis = 0;
        if (distance_to_face.y < distance_to_face[axis]) axis = 1;
        if (distance_to_face.z < distance_to_face[axis]) axis = 2;

        glm::vec3 normal_local{ 0.0f };
        normal_local[axis] = local_center[axis] >= 0.0f ? 1.0f : -1.0f;

        return worldize_vector(box, normal_local * (radius + distance_to_face[axis]));
    }

    [[nodiscard]]
    inline std::optional<glm::vec3> sphere_push_out(
        const glm::vec3&   center,
        const float        radius,
        const Transform&   transform,
        const BoxCollider& collider)
    {
        return sphere_push_out(center, radius, make_oriented_box(transform, collider));
    }

    inline void resolve_sphere_against_walls(
        glm::vec3&        center,
        const float       radius,
        const GameObject& walls_root)
    {
        if (!walls_root.valid()) return;

        for (int iteration = 0; iteration < 4; ++iteration)
        {
            bool moved = false;

            walls_root.for_each_child([&](GameObject wall)
            {
                if (!wall.valid() || !wall.active) return;

                const auto* transform = std::as_const(wall).try_get_component<Transform>();
                const auto* collider  = std::as_const(wall).try_get_component<BoxCollider>();

                if (!transform || !collider) return;

                const auto push = sphere_push_out(center, radius, *transform, *collider);
                if (!push.has_value()) return;

                glm::vec3 planar_push = *push;
                planar_push.y = 0.0f;

                if (glm::length2(planar_push) <= 1e-6f) return;

                center += planar_push;
                moved = true;
            });

            if (!moved) break;
        }
    }

    [[nodiscard]]
    inline std::optional<float> ray_hit_distance(
        const glm::vec3& origin,
        const glm::vec3& direction,
        const float      max_distance,
        const OrientedBox& box)
    {
        const glm::vec3 local_origin = localize_point(box, origin);
        const glm::vec3 local_direction{
            dot(direction, box.right),
            dot(direction, box.up),
            dot(direction, box.forward)
        };

        float t_min = 0.0f;
        float t_max = max_distance;

        constexpr float epsilon = 1e-6f;

        for (int axis = 0; axis < 3; ++axis)
        {
            if (std::abs(local_direction[axis]) <= epsilon)
            {
                if (local_origin[axis] < -box.extents[axis] ||
                    local_origin[axis] > box.extents[axis])
                    return std::nullopt;

                continue;
            }

            const float inv_dir = 1.0f / local_direction[axis];
            float t1 = (-box.extents[axis] - local_origin[axis]) * inv_dir;
            float t2 = ( box.extents[axis] - local_origin[axis]) * inv_dir;

            if (t1 > t2) std::swap(t1, t2);

            t_min = std::max(t_min, t1);
            t_max = std::min(t_max, t2);

            if (t_min > t_max) return std::nullopt;
        }

        if (t_min < 0.0f || t_min > max_distance) return std::nullopt;
        return t_min;
    }

    [[nodiscard]]
    inline std::optional<float> ray_hit_distance(
        const glm::vec3& origin,
        const glm::vec3& direction,
        const float      max_distance,
        const Transform& transform,
        const BoxCollider& collider)
    {
        return ray_hit_distance(origin, direction, max_distance, make_oriented_box(transform, collider));
    }
}

module game.terrain;

import :bounds;
import :config;

namespace game::terrain
{
    Bounds terrain_bounds(const Settings& settings)
    {
        return Bounds{ .min = glm::vec3{ 0.0f, 0.0f, 0.0f }, .max = compute_world_size(settings) };
    }

    Bounds chunk_bounds(const Settings& settings, const glm::uvec3& coord)
    {
        const glm::vec3 origin = chunk_origin(settings, coord);
        return Bounds{
            .min = origin,
            .max = origin + glm::vec3(settings.chunk_size) * voxel_size
        };
    }

    Bounds expanded_chunk_bounds(const Settings& settings, const glm::uvec3& coord)
    {
        const auto          [min, max] = chunk_bounds(settings, coord);
        constexpr glm::vec3 padding    = glm::vec3{ density_border } * voxel_size;

        return Bounds{
            .min = min - padding,
            .max = max + padding
        };
    }

    bool sphere_intersects_bounds(const glm::vec3& center, const float radius, const Bounds& bounds)
    {
        if (radius <= 0.0f) return false;

        const glm::vec3 closest = clamp(center, bounds.min, bounds.max);
        return glm::distance2(closest, center) <= radius * radius;
    }
}

module game.terrain:bounds;

import :components;

namespace game::terrain
{
    Bounds terrain_bounds(const Settings& settings);
    Bounds chunk_bounds(const Settings& settings, const glm::uvec3& coord);
    Bounds expanded_chunk_bounds(const Settings& settings, const glm::uvec3& coord);
    bool sphere_intersects_bounds(const glm::vec3& center, float radius, const Bounds& bounds);
}

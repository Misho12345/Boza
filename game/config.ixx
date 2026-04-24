export module game.config;

import boza.common;

export namespace game::config
{
    inline constexpr glm::uvec3 terrain_chunk_size{ 88u, 88u, 88u };
    inline constexpr glm::uvec3 terrain_chunk_counts{ 8u, 8u, 8u };
}

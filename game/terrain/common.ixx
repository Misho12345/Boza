export module game.terrain:common;

import std;
import boza;
using namespace boza;

export namespace game::terrain
{
    enum class EditOperation : std::uint32_t
    {
        Remove = 0u,
        Add = 1u
    };

    struct Settings final
    {
        glm::uvec3 chunk_size{ 1u, 1u, 1u };
        glm::uvec3 chunk_counts{ 1u, 1u, 1u };
    };

    struct EditCommand final
    {
        glm::vec3 world_center{ 0.0f, 0.0f, 0.0f };
        float radius{ 1.0f };
        EditOperation operation{ EditOperation::Remove };
    };
}

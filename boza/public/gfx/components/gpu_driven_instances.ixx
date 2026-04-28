module;

#include "api.hpp"

export module boza.gfx:gpu_driven_instances;

import std;

import :buffer;

export namespace boza
{
    class BOZA_API GpuDrivenInstances final
    {
    public:
        std::shared_ptr<Buffer> candidate_buffer{};
        std::shared_ptr<Buffer> shadow_candidate_buffer{};

        std::uint32_t candidate_count{ 0 };
        std::uint32_t shadow_candidate_count{ 0 };

        bool casts_shadows{ true };

        [[nodiscard]] const Buffer* forward_candidates() const
        {
            return candidate_buffer.get();
        }

        [[nodiscard]] const Buffer* shadow_candidates() const
        {
            if (shadow_candidate_buffer) return shadow_candidate_buffer.get();
            return candidate_buffer.get();
        }

        [[nodiscard]] std::uint32_t forward_count() const
        {
            return candidate_buffer ? candidate_count : 0u;
        }

        [[nodiscard]] std::uint32_t shadow_count() const
        {
            if (!casts_shadows) return 0u;
            if (!shadow_candidates()) return 0u;
            if (shadow_candidate_buffer && shadow_candidate_count > 0u) return shadow_candidate_count;
            return forward_count();
        }
    };
}

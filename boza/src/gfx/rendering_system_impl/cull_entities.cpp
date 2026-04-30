module boza.gfx;

import :rendering_system;
import :rendering_system_common;

import boza.core;

namespace boza
{
    void RenderingSystem::PrepareGpuCulling::execute()
    {
        assert_render_thread();

        if (!frame_active_) return;

        static std::uint32_t cull_log_frame = 0;
        const bool log_cull_frame = cull_log_frame < 4 || cull_log_frame % 5000 == 0;
        if (log_cull_frame)
        {
            Log::debug(
                "PrepareGpuCulling frame {}: frustum_valid={}",
                cull_log_frame,
                frustum_.valid);
        }

        if (frustum_.valid)
        {
            for (std::size_t i = 0; i < frustum_.planes.size(); ++i)
            {
                const FrustumPlane& plane = frustum_.planes[i];
                gpu_cull_frustum_planes_[i] = glm::vec4{ plane.normal, plane.distance };
            }
        }

        ++cull_log_frame;
    }
}

module boza.gfx;

import :rendering_system;
import :rendering_system_common;

namespace boza
{
    void RenderingSystem::SanityCheck::execute()
    {
        assert_render_thread();

        destroyed_meshes_this_frame_.clear();
        destroyed_materials_this_frame_.clear();

        clear_validity_caches();

        if (++sanity_frame_counter_ >= full_sanity_interval_)
            sanity_frame_counter_ = 0;
    }

    void RenderingSystem::clear_validity_caches()
    {
        valid_meshes_.clear();
        invalid_meshes_.clear();
        valid_materials_.clear();
        invalid_materials_.clear();
    }
}

module boza.gfx;

import :rendering_system;

namespace boza
{
    void RenderingSystem::ProcessInvalidated::execute(GameObject go, MeshRenderer& mr)
    {
        go.remove_component<tags::RenderCacheInvalidated>();

        if (mr.in_render_cache_) remove_from_render_cache(mr, go);

        if (destroyed_meshes_this_frame_.contains(mr.mesh_))
        {
            mr.mesh_ = nullptr;
            mr.mesh_name_.clear();
        }

        if (destroyed_materials_this_frame_.contains(mr.material_))
        {
            mr.material_ = nullptr;
            mr.material_name_.clear();
        }

        if (!mr.in_unresolved_cache_) insert_into_unresolved(mr, go);
    }
}

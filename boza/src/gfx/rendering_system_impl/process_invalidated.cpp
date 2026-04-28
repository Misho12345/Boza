module boza.gfx;

import :rendering_system;
import :rendering_system_common;

namespace boza
{
    void RenderingSystem::ProcessInvalidated::execute(GameObject go, MeshRenderer& mr)
    {
        assert_render_thread();

        go.remove_component<tags::RenderCacheInvalidated>();

        if (mr.in_render_cache_) remove_from_render_cache(mr, go);

        if (destroyed_meshes_this_frame_.contains(mr.mesh_)) mr.set_mesh_by_name({});
        if (destroyed_materials_this_frame_.contains(mr.material_)) mr.set_material_by_name({});

        if (go.has_component<GpuDrivenInstances>())
        {
            mr.cached_mesh_ = mr.mesh;
            mr.cached_material_ = mr.material;
            mr.mesh_ = mr.cached_mesh_;
            mr.material_ = mr.cached_material_;
            return;
        }

        if (!mr.in_unresolved_cache_) insert_into_unresolved(mr, go);
    }
}

module boza.gfx;

import :rendering_system;
import :rendering_system_common;

namespace boza
{
    void RenderingSystem::process_renderer_binding_change(
        const GameObject go,
        MeshRenderer&     mr,
        const bool        mesh_changed)
    {
        if (mr.in_render_cache_) remove_from_render_cache(mr, go);
        if (mr.in_unresolved_cache_) remove_from_unresolved(mr, go);

        if (mesh_changed) mr.dirty_mesh_ = false;
        else mr.dirty_material_ = false;

        Mesh*     mesh     = mr.mesh;
        Material* material = mr.material;

        if (go.has_component<GpuDrivenInstances>())
        {
            mr.cached_mesh_ = mesh;
            mr.cached_material_ = material;
            mr.mesh_ = mesh;
            mr.material_ = material;
            return;
        }

        if (mesh && material) insert_into_render_cache(mr, go, mesh, material);
        else insert_into_unresolved(mr, go);
    }

    void RenderingSystem::ProcessMeshChanged::execute(GameObject go, MeshRenderer& mr)
    {
        assert_render_thread();

        go.remove_component<tags::MeshChanged>();
        process_renderer_binding_change(go, mr, true);
    }

    void RenderingSystem::ProcessMaterialChanged::execute(GameObject go, MeshRenderer& mr)
    {
        assert_render_thread();

        go.remove_component<tags::MaterialChanged>();
        process_renderer_binding_change(go, mr, false);
    }

    void RenderingSystem::ProcessTransformChanged::execute(GameObject go, MeshRenderer& mr)
    {
        assert_render_thread();

        go.remove_component<tags::RenderTransformDirty>();

        if (go.has_component<GpuDrivenInstances>()) return;
        if (!mr.in_render_cache_ || !mr.cached_mesh_ || !mr.cached_material_) return;

        auto mat_it = render_cache_.find(mr.cached_material_);
        if (mat_it == render_cache_.end()) return;

        auto mesh_it = mat_it->second.mesh_buckets.find(mr.cached_mesh_);
        if (mesh_it == mat_it->second.mesh_buckets.end()) return;

        auto& bucket = mesh_it->second;
        auto elem_it = std::ranges::find_if(
            bucket.elements,
            [go](const RenderElement& element) { return element.entity == go; });
        if (elem_it == bucket.elements.end()) return;

        (void)refresh_render_element_candidate(*elem_it, mr.cached_mesh_, mr.cached_material_);
        bucket.candidates_dirty = true;
        bucket.shadow_candidates_dirty = true;
    }
}

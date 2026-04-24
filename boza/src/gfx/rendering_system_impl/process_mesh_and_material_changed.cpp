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
}

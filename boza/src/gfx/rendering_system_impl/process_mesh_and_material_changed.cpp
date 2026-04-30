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
        (void)go;
        (void)mr;
        (void)mesh_changed;

        // MeshRenderer changes are consumed directly by the GPU-driven render pass.
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

        (void)mr;
    }
}

module boza.gfx;

import :rendering_system;

import boza.rhi.render_context;
import boza.gfx.material_loader;

namespace boza
{
    void RenderingSystem::CameraUboUpdate::execute(const Camera& cam, const Transform& transform)
    {
        auto* camera_ubo = gfx::MaterialLoader::instance().camera_ubo();
        if (!camera_ubo)
        {
            frustum_.valid = false;
            return;
        }

        const float aspect_ratio = rhi::RenderContext::window()->aspect_ratio();

        const gfx::CameraUBO ubo_data{
            .view = transform.view_matrix(),
            .proj = cam.projection_matrix(aspect_ratio)
        };

        frustum_.set_from_view_projection(ubo_data.proj * ubo_data.view);

        camera_ubo->upload(&ubo_data, sizeof(gfx::CameraUBO), 0);
    }
}

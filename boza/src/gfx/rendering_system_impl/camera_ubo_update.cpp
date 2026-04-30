module boza.gfx;

import :rendering_system;
import :rendering_system_common;

import boza.rhi;
import :material_loader;

namespace boza
{
    void RenderingSystem::CameraUboUpdate::execute(const Camera& cam, const Transform& transform)
    {
        assert_render_thread();

        auto* camera_ubo = gfx::MaterialLoader::instance().camera_ubo();
        if (!camera_ubo)
        {
            frustum_.valid = false;
            return;
        }

        const auto* window = rhi::RenderContext::window();
        if (!window)
        {
            frustum_.valid = false;
            return;
        }

        const float aspect_ratio = window->aspect_ratio();

        const gfx::CameraUBO ubo_data{
            .view = transform.view_matrix(),
            .proj = cam.projection_matrix(aspect_ratio)
        };

        frustum_.set_from_view_projection(ubo_data.proj * ubo_data.view);

        camera_ubo->upload(&ubo_data, sizeof(gfx::CameraUBO), 0);

        gfx::MaterialLoader::instance().begin_light_update(
            glm::vec3{ transform.position },
            ubo_data.view,
            ubo_data.proj,
            window->width(),
            window->height(),
            cam.near_clip,
            cam.far_clip);
    }
}

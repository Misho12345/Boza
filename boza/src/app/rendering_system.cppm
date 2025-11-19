module boza.app:rendering_system;

import std;
import boza.ecs;
import boza.gfx;
import boza.rhi;
import boza.platform;

using boza::platform::Window;

namespace boza::app
{
    class RenderingSystem final
    {
    public:
        bool init(Window& window, std::shared_ptr<Scene> scene);
        void run();
        void destroy();

    private:
        std::shared_ptr<Scene> active_scene_{ nullptr };
        rhi::GraphicsApi api_{};
        Window* window_ = nullptr;

        std::unique_ptr<rhi::Instance> instance_{ nullptr };
        std::unique_ptr<rhi::Device> device_{ nullptr };
        std::unique_ptr<rhi::Swapchain> swapchain_{ nullptr };
        std::unique_ptr<rhi::DescriptorPool> descriptor_pool_{ nullptr };
    };
}

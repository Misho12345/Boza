export module boza.rhi:resource_cache;

import std;
import boza.common;
import boza.rhi.api;
import boza.rhi.objects;

export namespace boza::rhi
{
    class ResourceCache
    {
    public:
        template<typename T>
        struct GraphicsObjectDeleter
        {
            void operator()(T* ptr) const { delete ptr; }
        };

        using OwnedDescriptorSetLayout = std::unique_ptr<DescriptorSetLayout, GraphicsObjectDeleter<DescriptorSetLayout>>;

        ResourceCache() = default;
        ~ResourceCache() { clear(); }

        ResourceCache(const ResourceCache&) = delete;
        ResourceCache& operator=(const ResourceCache&) = delete;
        ResourceCache(ResourceCache&&) = delete;
        ResourceCache& operator=(ResourceCache&&) = delete;

        std::shared_ptr<ShaderModule> get_or_create_shader(
            const ShaderModuleDesc& desc,
            const std::function<std::unique_ptr<ShaderModule>(const ShaderModuleDesc&)>& factory);

        std::shared_ptr<Texture> get_or_create_texture(
            const std::string& path,
            const std::function<std::unique_ptr<Texture>(const std::string&)>& factory);

        struct GraphicsPipelineKey
        {
            std::string vertex_shader;
            std::string fragment_shader;
            std::size_t settings_hash{ 0 };

            bool operator==(const GraphicsPipelineKey&) const = default;

            struct Hash
            {
                size_t operator()(const GraphicsPipelineKey& key) const noexcept
                {
                    const size_t h1 = std::hash<std::string>{}(key.vertex_shader);
                    const size_t h2 = std::hash<std::string>{}(key.fragment_shader);
                    const size_t h3 = key.settings_hash;
                    return h1 ^ (h2 << 1) ^ (h3 << 2);
                }
            };
        };

        struct CachedGraphicsPipeline
        {
            std::unique_ptr<GraphicsPipeline> pipeline{};
            std::unique_ptr<PipelineLayout> layout{};
            std::vector<OwnedDescriptorSetLayout> descriptor_set_layouts{};
        };

        struct ComputePipelineKey
        {
            std::string compute_shader;

            bool operator==(const ComputePipelineKey&) const = default;

            struct Hash
            {
                size_t operator()(const ComputePipelineKey& key) const noexcept
                {
                    return std::hash<std::string>{}(key.compute_shader);
                }
            };
        };

        struct CachedComputePipeline
        {
            std::unique_ptr<ComputePipeline> pipeline{};
            std::unique_ptr<PipelineLayout> layout{};
            std::vector<OwnedDescriptorSetLayout> descriptor_set_layouts{};
        };

        [[nodiscard]]
        std::shared_ptr<const CachedGraphicsPipeline> get_cached_pipeline(
            const std::string& vert,
            const std::string& frag,
            std::size_t        settings_hash = 0);
        [[nodiscard]]
        std::shared_ptr<const CachedGraphicsPipeline> cache_graphics_pipeline(
            const std::string& vert,
            const std::string& frag,
            std::size_t        settings_hash,
            CachedGraphicsPipeline cached);

        [[nodiscard]]
        std::shared_ptr<const CachedComputePipeline> get_cached_compute_pipeline(const std::string& compute_shader);
        [[nodiscard]]
        std::shared_ptr<const CachedComputePipeline> cache_compute_pipeline(
            const std::string& compute_shader,
            CachedComputePipeline cached);

        void clear();

    private:
        struct ShaderKey
        {
            std::string path;
            ShaderStage stage;

            bool operator==(const ShaderKey&) const = default;

            struct Hash
            {
                size_t operator()(const ShaderKey& key) const noexcept
                {
                    const size_t h1 = std::hash<std::string>{}(key.path);
                    const size_t h2 = std::hash<int>{}(static_cast<int>(key.stage));
                    return h1 ^ (h2 << 1);
                }
            };
        };

        flat_map<ShaderKey, std::weak_ptr<ShaderModule>, ShaderKey::Hash> shader_cache_;
        flat_map<std::string, std::weak_ptr<Texture>> texture_cache_;
        flat_map<GraphicsPipelineKey, std::shared_ptr<CachedGraphicsPipeline>, GraphicsPipelineKey::Hash> pipeline_cache_;
        flat_map<ComputePipelineKey, std::shared_ptr<CachedComputePipeline>, ComputePipelineKey::Hash> compute_pipeline_cache_;

        mutable std::mutex shader_mutex_;
        mutable std::mutex texture_mutex_;
        mutable std::mutex pipeline_mutex_;
        mutable std::mutex compute_pipeline_mutex_;

        template<typename T, typename KeyType, typename MapType>
        std::shared_ptr<T> get_or_create(
            const KeyType&             key,
            MapType&                   cache,
            std::mutex&                mutex,
            const std::function<std::unique_ptr<T>()>& factory)
        {
            {
                std::lock_guard lock{ mutex };

                if (auto it = cache.find(key); it != cache.end())
                {
                    if (auto shared = it->second.lock()) return shared;
                }
            }

            auto unique_ptr = factory();
            if (!unique_ptr) return nullptr;

            auto created = std::shared_ptr<T>(unique_ptr.release(), GraphicsObjectDeleter<T>{});

            std::lock_guard lock{ mutex };

            if (auto it = cache.find(key); it != cache.end())
            {
                if (auto shared = it->second.lock()) return shared;
                it->second = created;
                return created;
            }

            cache[key] = created;
            return created;
        }
    };
}

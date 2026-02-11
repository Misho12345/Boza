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
        ResourceCache() = default;
        ~ResourceCache() { clear(); }

        ResourceCache(const ResourceCache&) = delete;
        ResourceCache& operator=(const ResourceCache&) = delete;
        ResourceCache(ResourceCache&&) = delete;
        ResourceCache& operator=(ResourceCache&&) = delete;

        std::shared_ptr<ShaderModule> get_or_create_shader(
            const ShaderModuleDesc& desc,
            const std::function<ShaderModule*(const ShaderModuleDesc&)>& factory);

        std::shared_ptr<Texture> get_or_create_texture(
            const std::string& path,
            const std::function<Texture*(const std::string&)>& factory);

        struct PipelineKey
        {
            std::string vertex_shader;
            std::string fragment_shader;
            std::size_t settings_hash{ 0 };

            bool operator==(const PipelineKey&) const = default;

            struct Hash
            {
                size_t operator()(const PipelineKey& key) const noexcept
                {
                    const size_t h1 = std::hash<std::string>{}(key.vertex_shader);
                    const size_t h2 = std::hash<std::string>{}(key.fragment_shader);
                    const size_t h3 = key.settings_hash;
                    return h1 ^ (h2 << 1) ^ (h3 << 2);
                }
            };
        };

        struct CachedPipeline
        {
            GraphicsPipeline* pipeline{ nullptr };
            PipelineLayout* layout{ nullptr };
            std::vector<DescriptorSetLayout*> descriptor_set_layouts;
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
            ComputePipeline* pipeline{ nullptr };
            PipelineLayout* layout{ nullptr };
            std::vector<DescriptorSetLayout*> descriptor_set_layouts;
        };

        CachedPipeline* get_cached_pipeline(const std::string& vert, const std::string& frag, std::size_t settings_hash = 0);
        void cache_pipeline(const std::string& vert, const std::string& frag, std::size_t settings_hash, const CachedPipeline& cached);

        CachedComputePipeline* get_cached_compute_pipeline(const std::string& compute_shader);
        void cache_compute_pipeline(const std::string& compute_shader, const CachedComputePipeline& cached);

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
        flat_map<PipelineKey, CachedPipeline, PipelineKey::Hash> pipeline_cache_;
        flat_map<ComputePipelineKey, CachedComputePipeline, ComputePipelineKey::Hash> compute_pipeline_cache_;

        mutable std::mutex shader_mutex_;
        mutable std::mutex texture_mutex_;
        mutable std::mutex pipeline_mutex_;
        mutable std::mutex compute_pipeline_mutex_;

        template<typename T, typename KeyType, typename MapType>
        std::shared_ptr<T> get_or_create(
            const KeyType&             key,
            MapType&                   cache,
            std::mutex&                mutex,
            const std::function<T*()>& factory)
        {
            std::lock_guard lock{ mutex };

            auto it = cache.find(key);
            if (it != cache.end())
            {
                if (auto shared = it->second.lock()) return shared;
            }

            T* raw_ptr = factory();
            if (!raw_ptr) return nullptr;

            auto shared = std::shared_ptr<T>(raw_ptr, [](T* ptr)
            {
                if (ptr)
                {
                    ptr->destroy();
                    delete ptr;
                }
            });

            cache[key] = shared;
            return shared;
        }
    };
}

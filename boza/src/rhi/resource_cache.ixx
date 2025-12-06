export module boza.rhi:resource_cache;

import std;
import boza.rhi.api;
import boza.rhi.objects;

export namespace boza::rhi
{
    class ResourceCache
    {
    public:
        ResourceCache() = default;
        ~ResourceCache() = default;

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

        void clear();

    private:
        struct ShaderKey
        {
            std::string path;
            ShaderStage stage;

            bool operator==(const ShaderKey&) const = default;
        };

        struct ShaderKeyHash
        {
            std::size_t operator()(const ShaderKey& key) const noexcept
            {
                const std::size_t h1 = std::hash<std::string>{}(key.path);
                const std::size_t h2 = std::hash<std::uint8_t>{}(static_cast<std::uint8_t>(key.stage));
                return h1 ^ (h2 << 1);
            }
        };

        std::unordered_map<ShaderKey, std::weak_ptr<ShaderModule>, ShaderKeyHash> shader_cache_;
        std::unordered_map<std::string, std::weak_ptr<Texture>> texture_cache_;

        mutable std::mutex shader_mutex_;
        mutable std::mutex texture_mutex_;

        template<typename T, typename KeyType, typename Hash = std::hash<KeyType>, typename Equal = std::equal_to<KeyType>, typename Alloc = std::allocator<std::pair<const KeyType, std::weak_ptr<T>>>>
        std::shared_ptr<T> get_or_create(
            const KeyType& key,
            std::unordered_map<KeyType, std::weak_ptr<T>, Hash, Equal, Alloc>& cache,
            std::mutex& mutex,
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


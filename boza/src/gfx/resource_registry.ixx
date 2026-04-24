export module boza.gfx.resource_registry;

import std;
import boza.common;
import boza.core;
import boza.gfx;

export namespace boza::gfx
{
    template <typename T>
    class ResourceRegistry
    {
    public:
        using map_type = mt::node_map<std::string, std::unique_ptr<T>>;

        // Individual registry calls are synchronized, but returned pointers or
        // references are not stable across concurrent mutation.
        [[nodiscard]]
        T* find_ptr(const std::string_view key)
        {
            std::scoped_lock lock{ mutex_ };
            warn_cross_thread_access();
            if (const auto it = resources_.find(key); it != resources_.end()) return it->second.get();
            return nullptr;
        }

        [[nodiscard]]
        const T* find_ptr(const std::string_view key) const
        {
            std::scoped_lock lock{ mutex_ };
            warn_cross_thread_access();
            if (const auto it = resources_.find(key); it != resources_.end()) return it->second.get();
            return nullptr;
        }

        [[nodiscard]]
        bool contains(const std::string_view key) const
        {
            std::scoped_lock lock{ mutex_ };
            warn_cross_thread_access();
            return resources_.contains(key);
        }

        [[nodiscard]]
        T& at(const std::string_view key)
        {
            std::scoped_lock lock{ mutex_ };
            warn_cross_thread_access();
            return *resources_.at(key);
        }

        [[nodiscard]]
        const T& at(const std::string_view key) const
        {
            std::scoped_lock lock{ mutex_ };
            warn_cross_thread_access();
            return *resources_.at(key);
        }

        template <typename U>
        std::pair<T*, bool> try_emplace(const std::string_view key, U&& value)
        {
            std::scoped_lock lock{ mutex_ };
            warn_cross_thread_access();
            std::string key_str{ key };

            if (const auto existing = resources_.find(key_str); existing != resources_.end())
            {
                return { existing->second.get(), false };
            }

            auto resource = std::make_unique<T>(std::forward<U>(value));
            T*   resource_ptr = resource.get();

            auto [it, inserted] = resources_.try_emplace(std::move(key_str), std::move(resource));
            if (inserted) valid_pointers_.insert(resource_ptr);

            return { it->second.get(), inserted };
        }

        bool erase(const std::string_view key)
        {
            std::scoped_lock lock{ mutex_ };
            warn_cross_thread_access();
            const auto it = resources_.find(key);
            if (it == resources_.end()) return false;
            if (it->second) valid_pointers_.erase(it->second.get());
            resources_.erase(it);
            return true;
        }

        [[nodiscard]]
        std::unique_ptr<T> take(const std::string_view key)
        {
            std::scoped_lock lock{ mutex_ };
            warn_cross_thread_access();
            const auto it = resources_.find(key);
            if (it == resources_.end()) return nullptr;

            auto resource = std::move(it->second);
            if (resource) valid_pointers_.erase(resource.get());
            resources_.erase(it);
            return resource;
        }

        void clear()
        {
            std::scoped_lock lock{ mutex_ };
            warn_cross_thread_access();
            valid_pointers_.clear();
            resources_.clear();
        }

        [[nodiscard]]
        bool exists(const T* ptr) const
        {
            std::scoped_lock lock{ mutex_ };
            warn_cross_thread_access();
            return ptr && valid_pointers_.contains(ptr);
        }

        [[nodiscard]]
        std::size_t size() const
        {
            std::scoped_lock lock{ mutex_ };
            warn_cross_thread_access();
            return resources_.size();
        }

    private:
        void warn_cross_thread_access() const
        {
            const std::thread::id current_thread = std::this_thread::get_id();
            if (first_access_thread_ == std::thread::id{})
            {
                first_access_thread_ = current_thread;
                return;
            }

            if (!cross_thread_access_warned_ && first_access_thread_ != current_thread)
            {
                Log::warn(
                    "ResourceRegistry is accessed from multiple threads; only individual calls are synchronized, and returned pointers or references require external lifetime synchronization"
                );
                cross_thread_access_warned_ = true;
            }
        }

        map_type resources_;
        flat_set<const T*> valid_pointers_;
        mutable std::mutex mutex_{};
        mutable std::thread::id first_access_thread_{};
        mutable bool cross_thread_access_warned_{ false };
    };
}

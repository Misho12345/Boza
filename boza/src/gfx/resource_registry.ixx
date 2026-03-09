export module boza.gfx.resource_registry;

import std;
import boza.common;
import boza.gfx;

export namespace boza::gfx
{
    template<typename T>
    class ResourceRegistry
    {
    public:
        using map_type = mt::node_map<std::string, std::unique_ptr<T>>;
        using iterator = typename map_type::iterator;
        using const_iterator = typename map_type::const_iterator;

        [[nodiscard]] iterator find(const std::string_view key) { return resources_.find(key); }
        [[nodiscard]] const_iterator find(const std::string_view key) const { return resources_.find(key); }

        [[nodiscard]] iterator end() { return resources_.end(); }
        [[nodiscard]] const_iterator end() const { return resources_.end(); }

        [[nodiscard]] bool contains(const std::string_view key) const { return resources_.contains(key); }

        [[nodiscard]] T& at(const std::string_view key) { return *resources_.at(key); }
        [[nodiscard]] const T& at(const std::string_view key) const { return *resources_.at(key); }

        [[nodiscard]] T* find_ptr(const std::string_view key)
        {
            if (const auto it = find(key); it != end()) return it->second.get();
            return nullptr;
        }

        [[nodiscard]] const T* find_ptr(const std::string_view key) const
        {
            if (const auto it = find(key); it != end()) return it->second.get();
            return nullptr;
        }

        template<typename U>
        std::pair<iterator, bool> try_emplace(const std::string_view key, U&& value)
        {
            std::string key_str{ key };

            if (const auto existing = resources_.find(key_str); existing != resources_.end()) return { existing, false };

            auto resource = std::make_unique<T>(std::forward<U>(value));
            const T* resource_ptr = resource.get();

            auto [it, inserted] = resources_.try_emplace(std::move(key_str), std::move(resource));
            if (inserted) valid_pointers_.insert(resource_ptr);

            return { it, inserted };
        }

        iterator erase(const iterator it)
        {
            if (it->second) valid_pointers_.erase(it->second.get());
            return resources_.erase(it);
        }

        bool erase(const std::string_view key)
        {
            const auto it = find(key);
            if (it == end()) return false;
            erase(it);
            return true;
        }

        void clear()
        {
            valid_pointers_.clear();
            resources_.clear();
        }

        [[nodiscard]] bool exists(const T* ptr) const { return ptr && valid_pointers_.contains(ptr); }
        [[nodiscard]] std::size_t size() const { return resources_.size(); }

    private:
        map_type resources_;
        flat_set<const T*> valid_pointers_;
    };
}
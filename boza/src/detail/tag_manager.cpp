module boza.detail;
import :tag_manager;

namespace boza::detail
{

    TagManager& TagManager::instance()
    {
        static TagManager instance;
        return instance;
    }

    void TagManager::register_tag(const std::string& name, const std::uint32_t id)
    {
        name_to_id_[name] = id;
        id_to_name_[id]   = name;
    }

    std::uint32_t TagManager::tag_id(const std::string& name) const
    {
        const auto it = name_to_id_.find(name);
        if (it != name_to_id_.end()) return it->second;

        // Log::warn("Tag '{}' not found, returning 0 (None)", name);
        return 0;
    }

    const std::string& TagManager::tag_name(const std::uint32_t id) const
    {
        const auto it = id_to_name_.find(id);
        if (it != id_to_name_.end()) return it->second;

        static const std::string unknown = "Unknown";
        return unknown;
    }

    bool TagManager::has_tag(const std::string& name) const { return name_to_id_.contains(name); }

    void TagManager::clear()
    {
        name_to_id_.clear();
        id_to_name_.clear();
    }
}
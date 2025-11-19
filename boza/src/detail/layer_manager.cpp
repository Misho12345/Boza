module boza.detail;
import :layer_manager;

namespace boza::detail
{
    LayerManager& LayerManager::instance()
    {
        static LayerManager instance;
        return instance;
    }

    void LayerManager::register_layer(const std::string& name, const std::uint32_t shift_amount)
    {
        const std::uint32_t mask = 1u << shift_amount;
        name_to_mask_[name] = mask;
        mask_to_name_[mask] = name;
    }

    std::uint32_t LayerManager::layer_mask(const std::string& name) const
    {
        const auto it = name_to_mask_.find(name);
        if (it != name_to_mask_.end()) { return it->second; }

        // Log::warn("Layer '{}' not found, returning 1 (Default)", name);
        return 1u;
    }

    const std::string& LayerManager::layer_name(const std::uint32_t mask) const
    {
        const auto it = mask_to_name_.find(mask);
        if (it != mask_to_name_.end()) return it->second;

        static const std::string unknown = "Unknown";
        return unknown;
    }

    bool LayerManager::has_layer(const std::string& name) const { return name_to_mask_.contains(name); }

    void LayerManager::clear()
    {
        name_to_mask_.clear();
        mask_to_name_.clear();
    }
}
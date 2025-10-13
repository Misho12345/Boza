#include "boza/core/LayerManager.hpp"
#include "boza/core/Logger.hpp"

namespace boza
{
    LayerManager& LayerManager::instance()
    {
        static LayerManager instance;
        return instance;
    }

    void LayerManager::register_layer(const std::string& name, const uint32_t shift_amount)
    {
        const uint32_t mask = 1u << shift_amount;
        name_to_mask_[name] = mask;
        mask_to_name_[mask] = name;
    }

    uint32_t LayerManager::get_layer_mask(const std::string& name) const
    {
        const auto it = name_to_mask_.find(name);
        if (it != name_to_mask_.end()) { return it->second; }

        Logger::warn("Layer '{}' not found, returning 1 (Default)", name);
        return 1u;
    }

    const std::string& LayerManager::get_layer_name(const uint32_t mask) const
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

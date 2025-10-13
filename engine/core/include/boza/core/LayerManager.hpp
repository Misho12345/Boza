#pragma once
#include "boza/pch.hpp"
#include <unordered_map>
#include <string>
#include <cstdint>

namespace boza
{
    class LayerManager
    {
    public:
        static LayerManager& instance();

        void register_layer(const std::string& name, uint32_t shift_amount);

        uint32_t get_layer_mask(const std::string& name) const;
        const std::string& get_layer_name(uint32_t mask) const;

        bool has_layer(const std::string& name) const;

        void clear();

    private:
        LayerManager() = default;

        std::unordered_map<std::string, uint32_t> name_to_mask_;
        std::unordered_map<uint32_t, std::string> mask_to_name_;
    };
}


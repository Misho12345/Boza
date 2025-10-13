#pragma once
#include "boza/pch.hpp"
#include <unordered_map>
#include <string>
#include <cstdint>

namespace boza
{
    class TagManager
    {
    public:
        static TagManager& instance();

        void register_tag(const std::string& name, uint32_t id);

        uint32_t get_tag_id(const std::string& name) const;
        const std::string& get_tag_name(uint32_t id) const;

        bool has_tag(const std::string& name) const;

        void clear();

    private:
        TagManager() = default;

        std::unordered_map<std::string, uint32_t> name_to_id_;
        std::unordered_map<uint32_t, std::string> id_to_name_;
    };
}

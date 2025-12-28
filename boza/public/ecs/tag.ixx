module;

#include "api.hpp"
#include <cstddef>

export module boza.ecs:tag;

import std;
import boza.common;

export namespace boza
{
    class BOZA_API Tag final
    {
    public:
        Tag() = default;
        explicit Tag(const std::uint32_t id) : id_{ id } {}
        explicit Tag(const std::string& name);

        Tag& operator=(const std::string& tag_name);
        Tag& operator=(std::uint32_t tag_id);

        PropertyGet<Tag, std::uint32_t> id{ &Tag::get_id, offsetof(Tag, id) };
        PropertyGet<Tag, std::string> name{ &Tag::get_name, offsetof(Tag, name) };

        bool operator==(const Tag& other) const;
        bool operator!=(const Tag& other) const;
        bool operator==(const std::string& tag_name) const;
        bool operator!=(const std::string& tag_name) const;

    private:
        [[nodiscard]] std::uint32_t get_id() const;
        [[nodiscard]] std::string get_name() const;

        uint32_t id_{ 0 };
    };
}
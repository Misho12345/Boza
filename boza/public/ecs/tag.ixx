module;

#include "api.hpp"

export module boza.ecs:tag;

import std;
import boza.common;

export namespace boza
{
    class BOZA_API Tag final
    {
        [[nodiscard]] std::uint32_t get_id() const;
        [[nodiscard]] const std::string& get_name() const;

    public:
        Tag() = default;
        explicit Tag(const std::uint32_t id) : id_{ id } {}
        explicit Tag(const std::string& name);

        Tag(const Tag& other);
        Tag& operator=(const Tag& other);

        Tag& operator=(const std::string& tag_name);
        Tag& operator=(std::uint32_t tag_id);

        [[msvc::no_unique_address]] Property<Tag, &Tag::get_id> id{ this };
        [[msvc::no_unique_address]] Property<Tag, &Tag::get_name> name{ this };

        bool operator==(const Tag& other) const;
        bool operator!=(const Tag& other) const;
        bool operator==(const std::string& tag_name) const;
        bool operator!=(const std::string& tag_name) const;

    private:
        uint32_t id_{ 0 };
    };
}
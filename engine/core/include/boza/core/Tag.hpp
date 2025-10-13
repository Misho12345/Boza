#pragma once
#include "boza/pch.hpp"
#include "Property.hpp"

namespace boza
{
    class TagManager;

    class Tag final
    {
    public:
        Tag() = default;
        explicit Tag(const uint32_t id) : id_{ id } {}
        explicit Tag(const std::string& name);

        Tag& operator=(const std::string& _name);
        Tag& operator=(uint32_t _id);

        PropertyGet<uint32_t> id{ GET { return id_; } };
        PropertyGet<std::string> name{ GET { return get_name(); } };

        bool operator==(const Tag& other) const;
        bool operator!=(const Tag& other) const;
        bool operator==(const std::string& _name) const;
        bool operator!=(const std::string& _name) const;

    private:
        std::string get_name() const;

        uint32_t id_{ 0 };
    };
}

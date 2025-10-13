#pragma once
#include "boza/pch.hpp"
#include "Property.hpp"

namespace boza
{
    class LayerManager;

    class Layer final
    {
    public:
        Layer() = default;
        explicit Layer(const uint32_t mask) : mask_{ mask } {}
        explicit Layer(const std::string& name);

        Layer& operator=(const std::string& _name);
        Layer& operator=(uint32_t _mask);

        PropertyGet<uint32_t> mask{ GET { return mask_; } };
        PropertyGet<std::string> name{ GET { return get_name(); } };

        Layer operator|(const Layer& other) const;
        Layer operator&(const Layer& other) const;
        Layer operator^(const Layer& other) const;
        Layer operator~() const;

        Layer& operator|=(const Layer& other);
        Layer& operator&=(const Layer& other);
        Layer& operator^=(const Layer& other);

        bool contains(const Layer& other) const;
        bool contains(const std::string& _name) const;

        bool operator==(const Layer& other) const;
        bool operator!=(const Layer& other) const;
        bool operator==(const std::string& _name) const;
        bool operator!=(const std::string& _name) const;

    private:
        std::string get_name() const;

        uint32_t mask_{ 1 };
    };
}


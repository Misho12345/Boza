module;

#include <cstddef>
#include "api.hpp"

export module boza.ecs:layer;

import std;
import boza.common;

export namespace boza
{
    class BOZA_API Layer final
    {
    public:
        Layer() = default;
        explicit Layer(const std::uint32_t mask) : mask_{ mask } {}
        explicit Layer(const std::string& name);

        Layer& operator=(const std::string& _name);
        Layer& operator=(std::uint32_t _mask);

        PropertyGet<Layer, std::uint32_t> mask{ &Layer::get_mask, offsetof(Layer, mask) };
        PropertyGet<Layer, std::string> name{ &Layer::get_name, offsetof(Layer, name) };

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
        std::uint32_t get_mask() const;
        std::string get_name() const;

        std::uint32_t mask_{ 1 };
    };
}
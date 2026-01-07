module;

#include "api.hpp"

export module boza.ecs:layer;

import std;
import boza.common;

export namespace boza
{
    class BOZA_API Layer final
    {
        [[nodiscard]] std::uint32_t get_mask() const;
        [[nodiscard]] const std::string& get_name() const;

    public:
        Layer() = default;
        explicit Layer(const std::uint32_t mask) : mask_{ mask } {}
        explicit Layer(const std::string& name);

        Layer(const Layer& other);
        Layer& operator=(const Layer& other);

        Layer& operator=(const std::string& layer_name);
        Layer& operator=(std::uint32_t mask_value);

        [[msvc::no_unique_address]] Property<Layer, &Layer::get_mask> mask{ this };
        [[msvc::no_unique_address]] Property<Layer, &Layer::get_name> name{ this };

        Layer operator|(const Layer& other) const;
        Layer operator&(const Layer& other) const;
        Layer operator^(const Layer& other) const;
        Layer operator~() const;

        Layer& operator|=(const Layer& other);
        Layer& operator&=(const Layer& other);
        Layer& operator^=(const Layer& other);

        [[nodiscard]] bool contains(const Layer& other) const;
        [[nodiscard]] bool contains(const std::string& layer_name) const;

        bool operator==(const Layer& other) const;
        bool operator!=(const Layer& other) const;
        bool operator==(const std::string& layer_name) const;
        bool operator!=(const std::string& layer_name) const;

    private:
        std::uint32_t mask_{ 1 };
    };
}
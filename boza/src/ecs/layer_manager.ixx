export module boza.ecs.layer_manager;

import std;
import boza.common;

export namespace boza::ecs
{
    class LayerManager final
    {
    public:
        static LayerManager& instance();

        void register_layer(const std::string& name, std::uint32_t shift_amount);

        [[nodiscard]] std::uint32_t layer_mask(const std::string& name) const;
        [[nodiscard]] const std::string& layer_name(std::uint32_t mask) const;

        [[nodiscard]] bool has_layer(const std::string& name) const;
        void clear();

    private:
        LayerManager() = default;

        flat_map<std::string, std::uint32_t> name_to_mask_{};
        flat_map<std::uint32_t, std::string> mask_to_name_{};
    };
}
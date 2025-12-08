module boza.app;

import :game_settings;

import std;
import boza.common;
import boza.core;
import boza.detail;

namespace boza::app
{
    using detail::TagManager;
    using detail::LayerManager;
    using detail::FileIO;

    bool GameSettings::load_from_file(const fs::path& filepath)
    {
        const auto data_opt = FileIO::load_json(filepath);
        if (!data_opt.has_value())
        {
            Log::error("Failed to load game settings file: {}", filepath.string());
            load_defaults();
            return false;
        }

        const auto& data = data_opt.value();

        if (data.contains("tags") && data["tags"].is_object())
        {
            auto& tag_manager = TagManager::instance();
            tag_manager.clear();

            for (auto& [name, id] : data["tags"].items())
            {
                if (id.is_number_unsigned()) tag_manager.register_tag(name, id.get<std::uint32_t>());
            }
        }

        if (data.contains("layers") && data["layers"].is_object())
        {
            auto& layer_manager = LayerManager::instance();
            layer_manager.clear();

            for (auto& [name, shift] : data["layers"].items())
            {
                if (shift.is_number_unsigned()) layer_manager.register_layer(name, shift.get<std::uint32_t>());
            }
        }

        return true;
    }

    void GameSettings::load_defaults()
    {
        Log::info("Loading default game settings");

        auto& tag_manager = TagManager::instance();
        tag_manager.clear();
        tag_manager.register_tag("None", 0);
        tag_manager.register_tag("Player", 1);
        tag_manager.register_tag("Enemy", 2);
        tag_manager.register_tag("Bullet", 3);

        auto& layer_manager = LayerManager::instance();
        layer_manager.clear();
        layer_manager.register_layer("Default", 0);
        layer_manager.register_layer("Ground", 1);
        layer_manager.register_layer("Wall", 2);
        layer_manager.register_layer("Player", 3);
    }
}

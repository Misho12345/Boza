module boza.app.game_settings;

import std;
import boza.common;
import boza.core;

import boza.detail;

namespace boza::app
{
    using detail::FileIO;

    WindowSettings GameSettings::window{};
    GameplaySettings GameSettings::gameplay{};

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

        if (data.contains("window") && data["window"].is_object())
        {
            const auto& w = data["window"];

            if (w.contains("width") && w["width"].is_number_unsigned())
            {
                window.width = w["width"].get<std::uint32_t>();
            }

            if (w.contains("height") && w["height"].is_number_unsigned())
            {
                window.height = w["height"].get<std::uint32_t>();
            }

            if (w.contains("title") && w["title"].is_string())
            {
                window.title = w["title"].get<std::string>();
            }

            if (w.contains("fullscreen") && w["fullscreen"].is_boolean())
            {
                window.fullscreen = w["fullscreen"].get<bool>();
            }
        }

        if (data.contains("gameplay") && data["gameplay"].is_object())
        {
            const auto& g = data["gameplay"];

            if (g.contains("target_fps") && g["target_fps"].is_number())
            {
                gameplay.target_fps = g["target_fps"].get<float>();
            }

            if (g.contains("vsync") && g["vsync"].is_boolean())
            {
                gameplay.vsync = g["vsync"].get<bool>();
            }

            if (g.contains("physics_update_rate") && g["physics_update_rate"].is_number())
            {
                gameplay.physics_update_rate = g["physics_update_rate"].get<float>();
            }
        }

        return true;
    }

    void GameSettings::load_defaults()
    {
        Log::info("Loading default game settings");

        window = WindowSettings{};
        gameplay = GameplaySettings{};
    }
}

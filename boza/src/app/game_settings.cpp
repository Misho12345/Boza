module boza.app.game_settings;

import std;
import boza.common;
import boza.core;

import boza.ecs.tag_manager;
import boza.ecs.layer_manager;

import boza.detail;

namespace boza::app
{
    using detail::FileIO;

    WindowSettings GameSettings::window{};
    GraphicsSettings GameSettings::graphics{};
    PhysicsSettings GameSettings::physics{};
    InputSettings GameSettings::input{};

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
            if (w.contains("width") && w["width"].is_number_unsigned()) window.width = w["width"].get<std::uint32_t>();
            if (w.contains("height") && w["height"].is_number_unsigned()) window.height = w["height"].get<std::uint32_t>();
            if (w.contains("title") && w["title"].is_string()) window.title = w["title"].get<std::string>();
            if (w.contains("fullscreen") && w["fullscreen"].is_boolean()) window.fullscreen = w["fullscreen"].get<bool>();
        }

        if (data.contains("graphics") && data["graphics"].is_object())
        {
            const auto& g = data["graphics"];
            if (g.contains("target_fps") && g["target_fps"].is_number()) graphics.target_fps = g["target_fps"].get<float>();
            if (g.contains("vsync") && g["vsync"].is_boolean()) graphics.vsync = g["vsync"].get<bool>();
        }

        if (data.contains("physics") && data["physics"].is_object())
        {
            const auto& p = data["physics"];
            if (p.contains("fixed_update_rate") && p["fixed_update_rate"].is_number())
                physics.fixed_update_rate = p["fixed_update_rate"].get<float>();
        }

        if (data.contains("input") && data["input"].is_object())
        {
            const auto& i = data["input"];
            if (i.contains("poll_rate") && i["poll_rate"].is_number())
                input.poll_rate = i["poll_rate"].get<float>();
        }

        if (data.contains("tags") && data["tags"].is_object())
        {
            auto& tag_manager = ecs::TagManager::instance();
            tag_manager.clear();

            for (auto& [name, id] : data["tags"].items())
            {
                if (id.is_number_unsigned()) tag_manager.register_tag(name, id.get<std::uint32_t>());
            }
        }

        if (data.contains("layers") && data["layers"].is_object())
        {
            auto& layer_manager = ecs::LayerManager::instance();
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

        window = WindowSettings{};
        graphics = GraphicsSettings{};
        physics = PhysicsSettings{};
        input = InputSettings{};

        auto& tag_manager = ecs::TagManager::instance();
        tag_manager.clear();
        tag_manager.register_tag("None", 0);
        tag_manager.register_tag("Player", 1);
        tag_manager.register_tag("Enemy", 2);

        auto& layer_manager = ecs::LayerManager::instance();
        layer_manager.clear();
        layer_manager.register_layer("Default", 0);
        layer_manager.register_layer("Ground", 1);
        layer_manager.register_layer("Player", 2);
        layer_manager.register_layer("Enemy", 3);
    }
}


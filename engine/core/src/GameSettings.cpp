#include "boza/core/GameSettings.hpp"

#include "boza/pch.hpp"

#include "boza/core/TagManager.hpp"
#include "boza/core/LayerManager.hpp"
#include "boza/core/Logger.hpp"

namespace boza
{
    bool GameSettings::load_from_file(const std::string& filepath)
    {
        try
        {
            std::ifstream file(filepath);
            if (!file.is_open())
            {
                Logger::error("Failed to open game settings file: {}", filepath);
                load_defaults();
                return false;
            }

            json data;
            file >> data;

            if (data.contains("tags") && data["tags"].is_object())
            {
                auto& tag_manager = TagManager::instance();
                tag_manager.clear();

                for (auto& [name, id] : data["tags"].items())
                {
                    if (id.is_number_unsigned())
                    {
                        tag_manager.register_tag(name, id.get<uint32_t>());
                        // Logger::trace("Registered tag: {} = {}", name, id.get<uint32_t>());
                    }
                }
            }

            if (data.contains("layers") && data["layers"].is_object())
            {
                auto& layer_manager = LayerManager::instance();
                layer_manager.clear();

                for (auto& [name, shift] : data["layers"].items())
                {
                    if (shift.is_number_unsigned())
                    {
                        layer_manager.register_layer(name, shift.get<uint32_t>());
                        // Logger::trace("Registered layer: {} = {} (mask: {})",
                        //             name, shift.get<uint32_t>(), 1u << shift.get<uint32_t>());
                    }
                }
            }

            // Logger::tr("Game settings loaded from: {}", filepath);
            return true;
        }
        catch (const json::parse_error& e)
        {
            Logger::error("Failed to parse game settings JSON: {}", e.what());
            load_defaults();
            return false;
        }
        catch (const std::exception& e)
        {
            Logger::error("Failed to load game settings: {}", e.what());
            load_defaults();
            return false;
        }
    }

    void GameSettings::load_defaults()
    {
        Logger::info("Loading default game settings");

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


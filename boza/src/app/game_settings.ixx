export module boza.app.game_settings;

import std;
import boza.common;

export namespace boza::app
{
    struct WindowSettings
    {
        std::uint32_t width{ 1280 };
        std::uint32_t height{ 720 };
        std::string title{ "Boza Engine" };
        bool fullscreen{ false };
    };

    struct GameplaySettings
    {
        float target_fps{ 60.0f };
        bool vsync{ true };
        float physics_update_rate{ 60.0f };
    };

    class GameSettings final
    {
    public:
        GameSettings() = delete;

        static bool load_from_file(const fs::path& filepath);
        static void load_defaults();

        static WindowSettings window;
        static GameplaySettings gameplay;
    };
}

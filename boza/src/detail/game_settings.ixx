export module boza.detail:game_settings;

import std;
import boza.common;

export namespace boza::detail
{
    struct WindowSettings
    {
        std::uint32_t width{ 1280 };
        std::uint32_t height{ 720 };
        std::string title{ "Boza Engine" };
        bool fullscreen{ false };
    };

    struct GraphicsSettings
    {
        float target_fps{ 60.0f };
        bool vsync{ true };
    };

    struct PhysicsSettings
    {
        float fixed_update_rate{ 60.0f };
    };

    struct InputSettings
    {
        float poll_rate{ 240.0f };
    };

    class GameSettings final
    {
    public:
        GameSettings() = delete;

        static bool load_from_file(const fs::path& filepath);
        static void load_defaults();

        static WindowSettings window;
        static GraphicsSettings graphics;
        static PhysicsSettings physics;
        static InputSettings input;
    };
}


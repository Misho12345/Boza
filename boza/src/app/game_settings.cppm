module boza.app:game_settings;

import std;
import boza.common;

namespace boza::app
{
    class GameSettings final
    {
    public:
        GameSettings() = delete;

        static bool load_from_file(const fs::path& filepath);
        static void load_defaults();
    };
}

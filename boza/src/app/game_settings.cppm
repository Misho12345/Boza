module boza.app:game_settings;
import std;

namespace boza::app
{
    class GameSettings final
    {
    public:
        GameSettings() = delete;

        static bool load_from_file(const std::string& filepath);
        static void load_defaults();
    };
}

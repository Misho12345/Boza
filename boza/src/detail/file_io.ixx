export module boza.detail:file_io;

import std;
import boza.common;

export namespace boza::detail
{
    class FileIO final
    {
    public:
        FileIO() = delete;

        static std::vector<std::uint8_t> read(const fs::path& path)
        {
            if (!fs::exists(path)) return {};
            std::ifstream file{ path, std::ios::ate | std::ios::binary };
            if (!file.is_open()) return {};

            std::vector<std::uint8_t> buffer{
                std::istreambuf_iterator(file),
                std::istreambuf_iterator<char>()
            };

            return buffer;
        }

        static void write(const fs::path& path, const std::span<std::uint8_t>& data)
        {
            if (!fs::exists(path.parent_path())) fs::create_directories(path.parent_path());
            std::ofstream file{ path, std::ios::binary };
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
        }
    };
}

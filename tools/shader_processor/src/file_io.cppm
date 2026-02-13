module shader_processor:file_io;

import :config;

namespace sp
{
    class FileIO final
    {
    public:
        FileIO() = delete;
        ~FileIO() = delete;

        static std::string read(const fs::path& path);
        static bool write(const fs::path& path, const std::string& data);
        static bool write(const fs::path& path, const std::vector<std::uint32_t>& data);
    };
}
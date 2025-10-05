#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace sp::utils
{
    class File final
    {
    public:
        File() = delete;
        ~File() = delete;

        static std::string read(const fs::path& path);
        static bool write(const fs::path& path, const std::string& data);
        static bool write(const fs::path& path, const std::vector<uint32_t>& data);
    };
}

#include "File.hpp"

#include <print>
#include <fstream>

namespace sp::utils
{
    std::string File::read(const fs::path& path)
    {
        std::ifstream file{ path, std::ios::binary };

        if (!file)
        {
            std::println("Failed to open file: {}", path.string());
            return {};
        }

        return { std::istreambuf_iterator(file), {} };
    }


    bool File::write(const fs::path& path, const std::string& data)
    {
        std::ofstream file{ path, std::ios::binary };

        if (!file)
        {
            std::println("Failed to open file: {}", path.string());
            return false;
        }

        file.write(data.data(), data.size());
        return true;
    }


    bool File::write(const fs::path& path, const std::vector<uint32_t>& data)
    {
        std::ofstream file{ path, std::ios::binary };

        if (!file)
        {
            std::println("Failed to open file: {}", path.string());
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(uint32_t));
        return true;
    }
}

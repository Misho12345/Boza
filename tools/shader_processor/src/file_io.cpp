module shader_processor;

import :file_io;

namespace sp
{
    std::string FileIO::read(const fs::path& path)
    {
        std::ifstream file{ path, std::ios::binary };

        if (!file)
        {
            std::println("Failed to open file: {}", path.string());
            return {};
        }

        return { std::istreambuf_iterator(file), {} };
    }


    bool FileIO::write(const fs::path& path, const std::string& data)
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


    bool FileIO::write(const fs::path& path, const std::vector<std::uint32_t>& data)
    {
        std::ofstream file{ path, std::ios::binary };

        if (!file)
        {
            std::println("Failed to open file: {}", path.string());
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(std::uint32_t));
        return true;
    }
}

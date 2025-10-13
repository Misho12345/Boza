#pragma once
#include <filesystem>
#include <string>
#include <unordered_set>

namespace fs = std::filesystem;

namespace sp
{
    /**
     * @class IncludeResolver
     * @brief Resolves and collects include dependencies in source files.
     */
    class IncludeResolver
    {
    public:
        IncludeResolver()  = delete;
        ~IncludeResolver() = delete;

        /**
         * @brief Recursively resolves all includes for a given shader file and returns a single source string.
         * @param entry The entry point shader file.
         * @return A string containing the pre-processed source code.
         */
        static std::string resolve(const fs::path& entry);

    private:
        static std::string resolve_recursive(
            const fs::path&                  file,
            std::unordered_set<std::string>& visited);
    };
}

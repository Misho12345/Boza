#include "IncludeResolver.hpp"
#include "utils/File.hpp"

#include <print>
#include <regex>

namespace sp
{
    using utils::File;

    std::string IncludeResolver::resolve(const fs::path& entry)
    {
        std::unordered_set<std::string> visited;
        return resolve_recursive(entry, visited);
    }

    std::string IncludeResolver::resolve_recursive(
        const fs::path&                  file,
        std::unordered_set<std::string>& visited)
    {
        const fs::path    abs = weakly_canonical(file);
        const std::string key = abs.string();
        if (visited.contains(key)) return "";
        visited.insert(key);

        const std::string raw = File::read(abs);
        if (raw.empty())
        {
            std::println(stderr, "IncludeResolver: failed to read {}", abs.string());
            return "";
        }

        static const std::regex include_re(R"(^\s*#\s*include\s*"([^"]+)\"\s*$)");
        std::string             processed_source;
        processed_source.reserve(raw.size());

        size_t start = 0;
        while (start < raw.size())
        {
            size_t end = raw.find('\n', start);
            if (end == std::string::npos) end = raw.size();
            const std::string_view line(raw.data() + start, end - start);

            if (std::cmatch m; std::regex_match(line.data(), line.data() + line.length(), m, include_re))
            {
                fs::path inc_path = abs.parent_path() / std::string(m[1].first, m[1].second);
                processed_source += resolve_recursive(inc_path, visited);
            }
            else
            {
                processed_source.append(line);
                processed_source.push_back('\n');
            }

            start = end + (end < raw.size() ? 1 : 0);
        }

        return processed_source;
    }
}

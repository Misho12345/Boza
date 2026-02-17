export module boza.core:assert;

import std;
import boza.common;
import :log;

namespace boza
{
    #ifdef _DEBUG
    const fs::path project_root{ BOZA_PROJECT_ROOT };

    inline std::string format_stacktrace(const std::stacktrace& trace)
    {
        std::string result;

        for (const auto& entry : trace)
        {
            const std::string description = entry.description();
            const std::string source_file = entry.source_file();
            const auto        source_line = entry.source_line();

            auto filename = fs::path{ source_file }.lexically_relative(project_root).string();
            result        += std::format("\t{} ({}:{})\n", description, filename, source_line);

            if (description == "main") break;
        }

        return result;
    }

    export template <typename... Args>
    void assert(const bool condition, std::format_string<Args...> fmt, Args... args)
    {
        if (condition) return;

        Log::critical(
            "Assertion failed: {}\n"
            "Call Stack:\n{}",
            std::format(fmt, std::forward<Args>(args)...),
            format_stacktrace(std::stacktrace::current(1)));

        std::abort();
    }
    #else
    export void assert(auto&&...) {}
    #endif
}

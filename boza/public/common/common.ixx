export module boza.common;

import std;

export import <nlohmann/json.hpp>;

export import :flags;
export import :random;

export import :property;
export import :global_property;

export import :glm;
export import :gtl;

export namespace boza
{
    namespace fs = std::filesystem;

    using namespace std::string_literals;
    using namespace std::string_view_literals;
    using namespace std::chrono_literals;
    using namespace std::complex_literals;

    using nlohmann::json;
}

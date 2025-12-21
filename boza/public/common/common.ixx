export module boza.common;

export import :property;
export import :flags;
export import :random;

export import glm;
export namespace glm
{
    using namespace gtc;
    using namespace gtx;
}

export namespace fs = std::filesystem;

export import <nlohmann/json.hpp>;
export using nlohmann::json;

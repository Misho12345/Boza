export module boza.common;

export import glm;
export import <nlohmann/json.hpp>;
import <gtl/phmap.hpp>;

export import :property;
export import :flags;
export import :random;

export namespace fs = std::filesystem;

export namespace glm
{
    using namespace gtc;
    using namespace gtx;
}

export namespace boza
{
    using nlohmann::json;

    template<typename... Args> using flat_map = gtl::flat_hash_map<Args...>;
    template<typename... Args> using flat_set = gtl::flat_hash_set<Args...>;
    template<typename... Args> using node_map = gtl::node_hash_map<Args...>;
    template<typename... Args> using node_set = gtl::node_hash_set<Args...>;

    namespace mt
    {
        template<typename... Args> using flat_map = gtl::parallel_flat_hash_map<Args...>;
        template<typename... Args> using flat_set = gtl::parallel_flat_hash_set<Args...>;
        template<typename... Args> using node_map = gtl::parallel_node_hash_map<Args...>;
        template<typename... Args> using node_set = gtl::parallel_node_hash_set<Args...>;
    }
}

export module boza.common:gtl;

import <gtl/phmap.hpp>;

export namespace boza
{
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

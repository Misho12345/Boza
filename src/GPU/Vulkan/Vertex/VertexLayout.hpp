#pragma once
#include "boza_pch.hpp"
#include <boost/pfr.hpp>

#define DEFINE_FORMAT(TYPE, VK_FORMAT) template<> struct format_of<TYPE>{ static constexpr VkFormat value = VK_FORMAT; }

#define DEFINE_FORMAT_GROUP(TYPE, SIZE, SUFFIX)                                           \
    DEFINE_FORMAT(glm::TYPE##SIZE      , VK_FORMAT_R##SIZE##_##SUFFIX);                   \
    DEFINE_FORMAT(glm::TYPE##SIZE##vec1, VK_FORMAT_R##SIZE##_##SUFFIX);                   \
    DEFINE_FORMAT(glm::TYPE##SIZE##vec2, VK_FORMAT_R##SIZE##G##SIZE##_##SUFFIX);          \
    DEFINE_FORMAT(glm::TYPE##SIZE##vec3, VK_FORMAT_R##SIZE##G##SIZE##B##SIZE##_##SUFFIX); \
    DEFINE_FORMAT(glm::TYPE##SIZE##vec4, VK_FORMAT_R##SIZE##G##SIZE##B##SIZE##A##SIZE##_##SUFFIX)

namespace boza
{
    template<typename T> struct format_of;

    DEFINE_FORMAT_GROUP(f, 32, SFLOAT);
    DEFINE_FORMAT_GROUP(f, 64, SFLOAT);

    DEFINE_FORMAT_GROUP(i, 8, SINT);
    DEFINE_FORMAT_GROUP(i, 16, SINT);
    DEFINE_FORMAT_GROUP(i, 32, SINT);
    DEFINE_FORMAT_GROUP(i, 64, SINT);

    DEFINE_FORMAT_GROUP(u, 8, UINT);
    DEFINE_FORMAT_GROUP(u, 16, UINT);
    DEFINE_FORMAT_GROUP(u, 32, UINT);
    DEFINE_FORMAT_GROUP(u, 64, UINT);

    struct VertexLayout
    {
        VkVertexInputBindingDescription                binding;
        std::vector<VkVertexInputAttributeDescription> attributes;
    };

    template<typename VertexT, std::size_t... I>
    VertexLayout make_layout_from_pfr(std::index_sequence<I...>)
    {
        VertexLayout layout;

        layout.binding.binding   = 0;
        layout.binding.stride    = static_cast<uint32_t>(sizeof(VertexT));
        layout.binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        uint32_t next_location = 0;
        VertexT  dummy{};

        bool error = false;

        (..., ([&]
        {
            if (error) return;
            using FieldT = std::remove_cvref_t<decltype(boost::pfr::get<I>(std::declval<VertexT>()))>;

            if constexpr (!std::is_same_v<FieldT, bool>)
            {
                if constexpr (requires { format_of<FieldT>::value; })
                {
                    const uint32_t offset =
                        reinterpret_cast<const char*>(&boost::pfr::get<I>(dummy)) -
                        reinterpret_cast<const char*>(&dummy);

                    layout.attributes.emplace_back(
                        next_location++,
                        0,
                        format_of<FieldT>::value,
                        offset
                    );
                }
                else
                {
                    Logger::critical("{} is not suitable -> {} {} is of unsupported type",
                        typeid(VertexT).name(),
                        typeid(FieldT).name(),
                        boost::pfr::get_name<I, VertexT>());

                    error = true;
                    layout.attributes.clear();
                }
            }
        }()));

        return layout;
    }


    template<typename VertexT>
    VertexLayout get_layout()
    {
        constexpr std::size_t N = boost::pfr::tuple_size_v<VertexT>;
        static_assert(N > 0, "Vertex type must have at least one field.");
        return make_layout_from_pfr<VertexT>(std::make_index_sequence<N>{});
    }
}

#pragma once
#include "Property.hpp"
#include "boza/pch.hpp"
#include "boza/API.hpp"

namespace boza
{
    struct Vertex
    {
        glm::packed_vec3 position;
        glm::packed_vec3 normal;
        glm::vec2        tex_coord;
    };

    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4251)
    #endif

    class BOZA_API Mesh
    {
    public:
        Mesh(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices);
        ~Mesh() = default;

        PropertyGet<std::vector<Vertex>>   vertices{ GET -> std::vector<Vertex>& { return vertices_; } };
        PropertyGet<std::vector<uint32_t>> indices{ GET -> std::vector<uint32_t>& { return indices_; } };
        PropertyGet<uint32_t>              index_count{ GET { return static_cast<uint32_t>(indices_.size()); } };

    private:
        std::vector<Vertex>   vertices_;
        std::vector<uint32_t> indices_;
    };

    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif
}

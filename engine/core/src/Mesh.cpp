#include "boza/core/Mesh.hpp"

namespace boza
{
    Mesh::Mesh(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices)
        : vertices_(std::move(vertices)), indices_(std::move(indices)) {}
}

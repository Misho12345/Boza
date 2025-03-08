#include "Mesh.hpp"

namespace boza
{
    Mesh::Mesh(Mesh&& other) noexcept :
        vertex_buffer(std::move(other.vertex_buffer)),
        index_buffer(std::move(other.index_buffer))
    {
        vertex_count = std::exchange(other.vertex_count, 0);
        index_count  = std::exchange(other.index_count, 0);
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept
    {
        if (this != &other)
        {
            vertex_buffer = std::move(other.vertex_buffer);
            index_buffer  = std::move(other.index_buffer);

            vertex_count  = std::exchange(other.vertex_count, 0);
            index_count   = std::exchange(other.index_count, 0);
        }

        return *this;
    }

    void Mesh::destroy()
    {
        vertex_buffer.destroy();
        index_buffer.destroy();
    }

    void Mesh::bind(const VkCommandBuffer command_buffer) const
    {
        constexpr std::array<VkDeviceSize, 1> offsets{};
        const VkBuffer buf = vertex_buffer.get_buffer();
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &buf, offsets.data());
        vkCmdBindIndexBuffer(command_buffer, index_buffer.get_buffer(), 0, VK_INDEX_TYPE_UINT32);
    }

    void  Mesh::draw(const VkCommandBuffer command_buffer) const
    {
        vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
    }
}

#include "Mesh.inl"